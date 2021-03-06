#pragma once

extern void ExportToObj(const char* path, const Mesh& m);

class Wind : public Scene
{
public:

    static const int AUTO_CLOTH_SIZE = -1;

    Mesh* obj;

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

    static void PrintProperties(ClothParams& cp) {
        printf(" ---- Cloth Params ----\n");
        printf("n_particles (per dim): %d\n", cp.n_particles);
        printf("particle_radius:       %f\n", cp.particle_radius);
        printf("mass:                  %f\n", cp.invMass);    
        printf("stretch_stiffness:     %f\n", cp.stretch_stiffness);
        printf("bend_stiffness:        %f\n", cp.bend_stiffness);
        printf("shear_stiffness:       %f\n", cp.shear_stiffness);
        printf("extra_cp_spacing:      %f\n", cp.extra_cp_spacing);
        printf("extra_cp_rad_mult:     %f\n", cp.extra_cp_rad_mult);
        cout << endl;
    }

    static void PrintProperties(ObjParams& op) {
        printf(" ---- Object Params ----\n");
        printf("scale:          %f\n", op.scale);
        printf("translate:      (%f, %f, %f)\n",
            op.translate.x,
            op.translate.y,
            op.translate.z);
        printf("rotate: (%f, %f, %f, %f)\n",
            op.rotate.x,
            op.rotate.y,
            op.rotate.z,
            op.rotate.w);
        printf("using quat?:    %s\n", (op.use_quat ? "yes" : "no"));

        cout << endl;
    }

    Wind(const char* name, const char* objpath, ClothParams cp, ObjParams op, float contact_eps=0.05f):
        Scene(name), objpath(objpath), cp(cp), op(op), contact_eps(contact_eps) {}


    virtual void Initialize() {
        PrintProperties(cp);
        PrintProperties(op);

        obj = ImportMesh(GetFilePathByPlatform(objpath).c_str());
        // obj->Normalize(op.scale); // Amir's change: do not normalize the shapes since our shapes are coming from ShapeNet Core and are already Normalized
        // obj->CenterAtOrigin();

        // <------ apply rotations
        if (op.use_quat) {
            auto x = op.rotate.x;
            auto y = op.rotate.y;
            auto z = op.rotate.z;
            auto w = op.rotate.w;
            // auto w = sqrt(1 - (x*x + y*y + z*z));
            obj->Transform(RotationMatrix(Quat(x, y, z, w)));

        } else {
            obj->Transform(RotationMatrix(op.rotate.x, Vec3(1.0f, 0.0f, 0.0f)));
            obj->Transform(RotationMatrix(op.rotate.y, Vec3(0.0f, 0.0f, -1.0f)));
            obj->Transform(RotationMatrix(op.rotate.z, Vec3(0.0f, 1.0f, 0.0f)));
        }

        // z & y directions swapped w.r.t. to render camera
        // obj->Transform(TranslationMatrix(Point3(op.translate.x, op.translate.z, op.translate.y))); // Amir's edit: commented this out since we are not using FleX for rendering

        Vec3 obj_lower, obj_upper;
        obj->GetBounds(obj_lower, obj_upper);
        printf("mesh upper: %f %f %f\n", obj_upper.x, obj_upper.y, obj_upper.z);
        printf("mesh lower: %f %f %f\n", obj_lower.x, obj_lower.y, obj_lower.z);

        NvFlexTriangleMeshId mesh = CreateTriangleMesh(obj, !g_renderOff);
        AddTriangleMesh(mesh, Vec3(), Quat(), 1.0f);
            // ^ the final parameter performs scaling from "local" to "world" space
            // the scaling factor was not reflected in upper/lower bounds below
            // since the scaling happens in "world space" but NvFlexGetTriangleMeshBounds
            // reflects "local space" coordinates

        /// Add cloth ///cloth_size: 160 # default 210


        // set cloth size (num particles)
        // int nx, ny, nz = 1;

        // <--- how many particles
        if (cp.n_particles == AUTO_CLOTH_SIZE) {
            // Vec2 min_cloth_dims = sphereDrape(obj_lower, obj_upper);
            Vec2 min_cloth_dims = bboxDrape(obj_lower, obj_upper);

            // add slack (for bboxDrape)
            min_cloth_dims += {0.1f, 0.1f};

            // convert
            nx = int(ceilf(min_cloth_dims.x / cp.particle_radius)); // TODO: fix (should be 2*radius)
            ny = int(ceilf(min_cloth_dims.y / cp.particle_radius)); // TODO: fix (should be 2*radius)

        } else {
            nx = cp.n_particles;
            ny = cp.n_particles;
        }

        // center cloth over object (remember, z & y swapped w.r.t. camera)
        //float fall_height = 0.01f;
        // z=-10 is higher in z_axis.
        Vec3 position_offset =Vec3(0.0f, 0.0f, -3.0f);
        //printf("%f \n %f \n %f\n", position_offset.x, position_offset.y, position_offset.z);

        // other per-particle properties
        float velocity = 0.0f;
        // float invMass = 3.0f;   // smaller values to be heavier
        //cout << cp.invMass <<endl;

        // create particle group for cloth (self-colliding)
        int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);

        // make cloth
        cloth_base_idx = int(g_buffers->positions.size());



        // CreateSpringGrid(position_offset, nx, ny, 1, cp.particle_radius, phase,
        //     cp.stretch_stiffness, cp.bend_stiffness, cp.shear_stiffness, velocity, invMass,
  //           cp.extra_cp_spacing, cp.extra_cp_rad_mult);
        


        CreateSpringGrid(position_offset, nx, ny, 1, cp.particle_radius, phase,
            cp.stretch_stiffness, cp.bend_stiffness, cp.shear_stiffness, velocity, cp.invMass,
            cp.extra_cp_spacing, cp.extra_cp_rad_mult);
        

        Vec3 cloth_upper;
        Vec3 cloth_lower;

        GetParticleBounds(cloth_lower, cloth_upper);
        printf("cloth lower: %f %f %f\n", cloth_lower.x, cloth_lower.y, cloth_lower.z);
        printf("cloth upper: %f %f %f\n", cloth_upper.x, cloth_upper.y, cloth_upper.z);

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

            g_buffers->velocities[i] = RandomUnitVector()*0.01f;    //<--- large values will generate grins on the cloth

            float minSqrDist = FLT_MAX;

            if (i != c1 && i != c2)
            {
                float stiffness = 0.8f;   //-0.8f
                float give = 0.1f;    //0.1

                float Dist_c1 = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[i]));
                float Dist_c2 = LengthSq(Vec3(g_buffers->positions[c2])-Vec3(g_buffers->positions[i]));
                float sqrDist = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[c2]));

                if (sqrDist < minSqrDist) {
                    // Add CreateSpring make the cloth more wrinkles
                    // negative, closer to 0 to be stiffner
                    CreateSpring(c1, i, -stiffness*exp(-Dist_c1/0.2f), give);
                    CreateSpring(c2, i, -stiffness*exp(-Dist_c2/0.2f), give);
                    //CreateSpring(c1, i, -stiffness, give);
                    //CreateSpring(c2, i, -stiffness, give);
                    minSqrDist = sqrDist;
                }
            }
        }

        
        if (!g_params.drag) {
            //cout << "@@@@@@@@@@@@@@ Warning! g_params.drag is not defined... " <<endl;
            g_params.drag = 0.05f;
        }        

        if (!g_params.numIterations) {
            //cout << "@@@@@@@@@@@@@@ Warning! g_params.numIterations is not defined... " <<endl;
            g_params.numIterations = 8;
        }

        if (!g_params.dynamicFriction) {
            //cout << "@@@@@@@@@@@@@@ Warning! g_params.dynamicFriction is not defined... " <<endl;
            g_params.dynamicFriction = 0.1625f;

        }    

        if (!g_params.collisionDistance) {
            //cout << "@@@@@@@@@@@@@@ Warning! g_params.collisionDistance is not defined... " <<endl;
            g_params.collisionDistance = cp.particle_radius;
        }

        if (!g_params.shapeCollisionMargin) {
            //cout << "@@@@@@@@@@@@@@ Warning! g_params.shapeCollisionMargin is not defined... " <<endl;
            g_params.shapeCollisionMargin = cp.particle_radius*0.1f;
        }

        //g_params.relaxationFactor = 1.3f;
        //g_windStrength = 2.0f;
        g_params.staticFriction = 3.18f; // Amir's edit: Added this line. Default value would be 0.0f is this line is removed

        // draw options
        g_drawPoints = false;
        g_drawSprings = false;
    }

    float ComputeMaxNeighborDist() {

        const auto& particles = g_buffers->positions;
        float max_dist = 0.0f;
        int base = cloth_base_idx;
        Vec3 center;

        // Middle vertices
        for (int y=1; y < ny; ++y) {
            for (int x=1; x < nx - 1; ++x) {
                center = Vec3(particles[base + GridIndex(x, y, nx)]);
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(x-1, y-1, nx)]))); // top left
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(x, y-1, nx)])));   // top
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(x+1, y-1, nx)]))); // top right
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(x-1, y, nx)])));   // left
            }
        }

        // Edge cases
        for (int x = 1; x < nx; ++x) { // top row
            center = Vec3(particles[base + GridIndex(x, 0, nx)]);
            max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(x-1, 0, nx)]))); // left
        }
        if (nx > 1) {
            for (int y = 1; y < ny; ++y) { // left/right columns
                center = Vec3(particles[base + GridIndex(0, y, nx)]);
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(0, y-1, nx)]))); // top
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(1, y-1, nx)]))); // top right

                center = Vec3(particles[base + GridIndex(nx-1, y, nx)]);
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(nx-2, y-1, nx)]))); // top left
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(nx-2, y, nx)])));   // left
                max_dist = max(max_dist, Length(center - Vec3(particles[base + GridIndex(nx-1, y-1, nx)]))); // top
            }
        }

        return max_dist;
    }

    void Destroy() {
        delete obj;
    }

    void Export(const char* basename) {

        char clothPath[400];
        if (g_saveClothPerSimStep) {
            sprintf(clothPath, "%s_cloth_%d.obj", basename, g_frame);
        } else {
            sprintf(clothPath, "%s_cloth.obj", basename);
        }
        /*
        Amir's edit
        char meshPath[300];
        sprintf(meshPath, "%s_mesh.obj", basename);
        char contactsPath[300];
        sprintf(contactsPath, "%s_contacts.txt", basename);
        */

        printf("Exporting cloth to %s\n", clothPath);

        ofstream obj_file;
        obj_file.open(clothPath);

        if (!obj_file) {
            printf("Failed to write to %s\n", clothPath);
            return;
        }

        char line[300];

        auto& particles = g_buffers->positions;
        obj_file << "# Vertices\n";
        obj_file << "o cloth\n";
        for (int i = 0; i < g_buffers->positions.size(); i++)
        {
            sprintf(line, "v %f %f %f\n", particles[i].x, particles[i].y, particles[i].z);  // <------ [XiaoLvXi]: cloth position
            obj_file << line;
        }

        // These are not vertex normals, they are face normals
        //
        // auto& norms = g_buffers->triangleNormals;
        // obj_file << "\n# Vertex normals\n";
        // for (int i = 0; i < norms.size(); i++)
        // {
        //     sprintf(line, "vn %f %f %f\n", norms[i].x, norms[i].y, norms[i].z);
        //     obj_file << line;
        // }

        auto& tris = g_buffers->triangles;
        obj_file << "\n# Faces\n";
        for (int i = 0; i < tris.size()/3; i++) {
            sprintf(line, "f %d %d %d\n", tris[i*3+0]+1, tris[i*3+1]+1, tris[i*3+2]+1);
            obj_file << line;
        }

        obj_file.close();

        /*
        Amir's edit: we don't need to save the mesh objs. We only need the cloth obj file and the previous lines take care of that
        printf("Exporting mesh to %s\n", meshPath);

        ExportToObj(&meshPath[0], *obj);
        */
        


    }

    void Update() {
        // DetectTears(); // Amir's edit: disable doing unnecessary calculations
        return ;
    }


    void DetectTears() {
        auto& particles = g_buffers->positions;
        // auto& normals = g_buffers->triangleNormals;
        // auto& tris = g_buffers->triangles;

        // normals check
        // printf("Normals size: %zu\nTriangles size: %zu\n", normals.size(), tris.size());
        // for (int i = 0; i < normals.size(); ++i) {
        //     int base = 3*i;
        //     Vec3& n = normals[i];

        // }

        // height (y-axis) check
        float cloth_height = -FLT_MAX;
        for (int i = 0; i < particles.size(); ++i) {
            auto& p = particles[i];
            cloth_height = (cloth_height < p.y) ? p.y : cloth_height;
        }

        Vec3 lower, upper;
        obj->GetBounds(lower, upper);

        if (upper.y > cloth_height) {
            printf("Tear detected: height violation.\n");
            g_quit = true;
        }
    }




private:

    const char* objpath;

    ClothParams cp;
    ObjParams op;
    int cloth_base_idx;
    int nx, ny;

    float contact_eps; // max distance for cloth-mesh "contact"

    // Returns cloth dimensions to fully drape bounding box of mesh object
    Vec2 bboxDrape(Vec3 mesh_lower, Vec3 mesh_upper) {
        Vec3 d = mesh_upper - mesh_lower;
        // remember, z & y are switched w.r.t camera (y-axis is height)
        float lx = d.x + 2 * d.y;
        float ly = d.z + 2 * d.y;
        return Vec2(lx, ly);
    }
};
