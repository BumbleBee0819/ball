#pragma once

extern void ExportToObj(const char* path, const Mesh& m);


class Ball : public Scene
{

public:

    static const int AUTO_CLOTH_SIZE = -1;

    Mesh* obj;
    Mesh* slope;
    float mTime;
    Vec3 obj_center;
    Vec3 obj_lower;


    struct ClothParams {
        int n_particles;
        float particle_radius;
        float invMass;
        float stretch_stiffness;
        float bend_stiffness;
        float shear_stiffness;
        float extra_cp_spacing;
        float extra_cp_rad_mult;

        ClothParams(int cs=64, float pr=0.05f, float mass=1.0, float sts=1.0f, float bs=0.8f, float shs=0.5f, float es=0.0f, float erm=1.0f):
            n_particles(cs), particle_radius(pr), invMass(mass), stretch_stiffness(sts),
            bend_stiffness(bs), shear_stiffness(shs), extra_cp_spacing(es), extra_cp_rad_mult(erm) {}
    };


    struct ObjParams {
        float scale;
        Vec3 translate;
        Vec4 rotate;
        bool use_quat = true;

        ObjParams(float scale=1.0, Vec3 t=Vec3(0,0,0), Vec4 r=Vec4(0,0,0,0), bool use_quat=true):
            scale(scale), translate(t), rotate(r), use_quat(use_quat) {}
    };



    Ball(const char* name, const char* objpath, ClothParams cp, ObjParams op, float contact_eps=0.05f):
        Scene(name), objpath(objpath), cp(cp), op(op), contact_eps(contact_eps) {}




    void Initialize() {
        int group = 0;

        slope = ImportMesh(GetFilePathByPlatform("/home/wbi/Code/Cloth_Project/flex_cloth/crossdomain_cloth_perception/dataset/trialObjs/ball/land.obj").c_str());
        NvFlexTriangleMeshId mesh = CreateTriangleMesh(slope, !g_renderOff);
        AddTriangleMesh(mesh, Vec3(), Quat(), 1.0f);


        // Import object Mesh //
        obj = ImportMesh(GetFilePathByPlatform(objpath).c_str());
        obj -> Transform(TranslationMatrix(Point3(op.translate.x, op.translate.z, op.translate.y))); // Amir's edit: commented this out since we are not using FleX for rendering

        Vec3 obj_lower, obj_upper;
        obj -> GetBounds(obj_lower, obj_upper);


        // Add cloth ///
        nx = cp.n_particles;
        ny = cp.n_particles;

        // center cloth over object (remember, z & y swapped w.r.t. camera)
        float fall_height = 1.0f;
        Vec3 obj_center = 0.5f * (obj_upper + obj_lower);


        Vec3 position_offset = Vec3(
            obj_center.x - 0.5 * nx * (cp.extra_cp_rad_mult*cp.particle_radius + cp.extra_cp_spacing),
            obj_upper.y + fall_height,
            obj_center.z - 0.5 * ny * (cp.extra_cp_rad_mult*cp.particle_radius + cp.extra_cp_spacing)
        );


        float velocity = 0.0f;
        int phase = NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide);


        //make cloth
        cloth_base_idx = int(g_buffers->positions.size());

        CreateSpringGrid(position_offset, nx, ny, 1, cp.particle_radius, phase,
            cp.stretch_stiffness, cp.bend_stiffness, cp.shear_stiffness, velocity, cp.invMass,
            cp.extra_cp_spacing, cp.extra_cp_rad_mult);


        int cloth_n = int(g_buffers->positions.size());
        

        Vec3 cloth_upper, cloth_lower;
        GetParticleBounds(cloth_lower, cloth_upper);


        g_params.radius = cp.particle_radius*1.0f;

        const int c1 = 0;
        const int c2 = nx-1;

        g_buffers->positions[c1].w = 0.0f;
        g_buffers->positions[c2].w = 0.0f;


        // add tethers
        for (int i=0; i < int(g_buffers->positions.size()); ++i)
        {
            // hack to rotate cloth
            swap(g_buffers->positions[i].y, g_buffers->positions[i].z);
            g_buffers->positions[i].y *= -1.0f;

            g_buffers->velocities[i] = RandomUnitVector()*0.01f;

            float minSqrDist = FLT_MAX;

            if (i != c1 && i != c2)
            {
                float stiffness = 0.8f;   //-0.8f
                float give = 0.1f;    //0.1

                float Dist_c1 = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[i]));
                float Dist_c2 = LengthSq(Vec3(g_buffers->positions[c2])-Vec3(g_buffers->positions[i]));
                float sqrDist = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[c2]));

                if (sqrDist < minSqrDist) {
                    CreateSpring(c1, i, -stiffness*exp(-Dist_c1/0.2f), give);
                    CreateSpring(c2, i, -stiffness*exp(-Dist_c2/0.2f), give);
                    minSqrDist = sqrDist;
                }
            }
        }



        // // // add object // 
        obj_start_index = g_buffers->positions.size();

        int obj_phase=NvFlexMakePhase(group++, 0);
        int obj_numvertices = int(obj->m_positions.size());


        if (g_buffers->rigidIndices.empty())
            g_buffers->rigidOffsets.push_back(0);

        for (int i=0;i<obj_numvertices;++i) {
            g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));
            g_buffers->positions.push_back(Vec4(obj->m_positions[i].x, obj->m_positions[i].y, obj->m_positions[i].z, 0.001f));
            // Vec3(0.0f, 0.0f, 10.0f): move towards the cloth
            g_buffers -> velocities.push_back(Vec3(0.0f,-1.0f,1.2f));
            g_buffers -> phases.push_back(obj_phase);
        }

        g_buffers->rigidCoefficients.push_back(1.0f);
        g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));


        g_params.staticFriction = 3.18f;


    }








    void Destroy() {
        delete obj;
    }



    void Export(const char* basename) {

        char clothPath[400];
        char objPath[400];
        ofstream obj_file;
        char line[300];

        if (g_saveClothPerSimStep) {
            sprintf(clothPath, "%s_cloth_%d.obj", basename, g_frame);
            sprintf(objPath, "%s_obj_%d.obj", basename, g_frame);
        }
        else {
            sprintf(clothPath, "%s_cloth.obj", basename);
            sprintf(objPath, "%s_obj_%d.obj", basename);
        }

        auto& particles = g_buffers->positions;

        printf("Exporting cloth to %s\n", clothPath);
        


        obj_file.open(clothPath);

        if (!obj_file) {
            printf("Failed to write to %s\n", clothPath);
            return;
        }
 
        obj_file << "o cloth\n";
        for (int i = 0; i < obj_start_index; i++)
        {
            sprintf(line, "v %f %f %f\n", particles[i].x, particles[i].y, particles[i].z);
            obj_file << line;
        }


        auto& tris = g_buffers->triangles;
        obj_file << "\ns off\n";
        for (int i = 0; i < tris.size()/3; i++)
        {
            sprintf(line, "f %d %d %d\n", tris[i*3+0]+1, tris[i*3+1]+1, tris[i*3+2]+1);
            obj_file << line;
        }
        // obj_file.close();


        // obj_file.open(objPath);

        // if (!obj_file) {
        //     printf("Failed to write to %s\n", objPath);
        //     return;
        // }

        obj_file << "o object\n";

        for (int i = obj_start_index; i < g_buffers->positions.size(); i++)
        {
            sprintf(line, "v %f %f %f\n", particles[i].x, particles[i].y, particles[i].z); 
            obj_file << line;
        }    


        int obj_numFaces = int(obj->GetNumFaces());
        obj_file << "\ns off\n";

        for (int i=0; i< obj_numFaces; ++i) {
            sprintf(line, "f %d %d %d\n", obj->m_indices[i*3]+1+obj_start_index, obj->m_indices[i*3+1]+1+obj_start_index, obj->m_indices[i*3+2]+1+obj_start_index);
            obj_file << line;
        }

        obj_file.close();



        // char meshPath[300];
        // sprintf(meshPath, "%s_mesh.obj", basename);
        // ExportToObj(&meshPath[0], *slope);


    }



    void Update() {
    }









private:

    const char* objpath;
    ClothParams cp;
    ObjParams op;

    int cloth_base_idx;
    int obj_start_index;
    int nx, ny;

    float contact_eps; // max distance for cloth-mesh "contact"

    // Returns cloth dimensions to fully drape mesh object in all radial
    //     directions, i.e. drape the smallest enclosing sphere
    Vec2 sphereDrape(Vec3 mesh_lower, Vec3 mesh_upper) {
        float width = 0.0f;

        // // drape top half of sphere
        float diam = Mag(mesh_upper - mesh_lower);
        float circ = diam * kPi;
        width += circ / 2.0f;

        // add enough to touch the ground regardless of topmost contact point
        width += 2 * diam;

        return Vec2(width, width);
    }

    // Returns cloth dimensions to fully drape bounding box of mesh object
    Vec2 bboxDrape(Vec3 mesh_lower, Vec3 mesh_upper) {
        Vec3 d = mesh_upper - mesh_lower;

        // remember, z & y are switched w.r.t camera (y-axis is height)
        float lx = d.x + 2 * d.y;
        float ly = d.z + 2 * d.y;

        return Vec2(lx, ly);
    }
};
