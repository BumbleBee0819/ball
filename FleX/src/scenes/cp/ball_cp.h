#pragma once

extern void ExportToObj(const char* path, const Mesh& m);


class Ball : public Scene
{

public:

    static const int AUTO_CLOTH_SIZE = -1;

    Mesh* obj;
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

    virtual void Initialize()
    {
        int group = 0;
        obj_start_index = g_buffers->positions.size();


        // /// Add object ///
        obj = ImportMesh(GetFilePathByPlatform(objpath).c_str());
        obj->Transform(TranslationMatrix(Point3(op.translate.x, op.translate.z, op.translate.y))); // Amir's edit: commented this out since we are not using FleX for rendering

        // // invert box faces
        // for (int i=0; i < int(obj->GetNumFaces()); ++i)
        // 	swap(obj->m_indices[i*3+0], obj->m_indices[i*3+1]);	

        Vec3 obj_lower, obj_upper;
        obj->GetBounds(obj_lower, obj_upper);


        int obj_phase=NvFlexMakePhase(group++, 0);
        int obj_numvertices = int(obj->m_positions.size());

        if (g_buffers->rigidIndices.empty())
			g_buffers->rigidOffsets.push_back(0);

        for (int i=0;i<obj_numvertices;++i) {
        	g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));
        	g_buffers->positions.push_back(Vec4(obj->m_positions[i].x, obj->m_positions[i].y, obj->m_positions[i].z, 1.0f));
        	g_buffers->velocities.push_back(Vec3(0.0f, 0.0f, 0.0f));
        	g_buffers->phases.push_back(obj_phase);
        }

        g_buffers->rigidCoefficients.push_back(1.0f);
		g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));




        

        
        // int obj_numvertices = int(obj->m_positions.size());
        // float* obj_vertices = new float [obj_numvertices*3];

        // for (int i=0;i<obj_numvertices;++i) {
        // 	obj_vertices[i*3]=float(obj->m_positions[i].x);
        // 	obj_vertices[i*3+1]=float(obj->m_positions[i].y);
        // 	obj_vertices[i*3+2]=float(obj->m_positions[i].z);
        //  }



        // int obj_numfaces = 3*int(obj->GetNumFaces());
        // int* obj_faces = new int[obj_numfaces];

        // for (int i=0; i<obj_numfaces; ++i) {
        // 	obj_faces[i] = int(obj->m_indices[i]);
        // }


        // NvFlexExtAsset* asset = NvFlexExtCreateRigidFromMesh(obj_vertices,
        // 	obj_numvertices,
        // 	obj_faces,
        // 	obj_numfaces/3,
        // 	0.02,
        // 	1.0); 

        // cout << asset->particles << endl;

        // delete obj_vertices;
        // delete obj_faces;






        // /// Add cloth ///
        // nx = cp.n_particles;
        // ny = cp.n_particles;

        // // center cloth over object (remember, z & y swapped w.r.t. camera)
        // float fall_height = 1.0f;
        // Vec3 obj_center = 0.5f * (obj_upper + obj_lower);


        // Vec3 position_offset = Vec3(
        //     // red: x
        //     obj_center.x - 0.5 * nx * (cp.extra_cp_rad_mult*cp.particle_radius + cp.extra_cp_spacing),
        //     // green: far away from the ball
        //     obj_upper.y + fall_height,
        //     // blue: z
        //      obj_center.z - 0.5 * ny * (cp.extra_cp_rad_mult*cp.particle_radius + cp.extra_cp_spacing)
        // );


        // // other per-particle properties
        // float velocity = 0.0f;

        // // create particle group for cloth (self-colliding)
        // int phase = NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide);

        // // make cloth
        // cloth_base_idx = int(g_buffers->positions.size());
        // CreateSpringGrid(position_offset, nx, ny, 1, cp.particle_radius, phase,
        //     cp.stretch_stiffness, cp.bend_stiffness, cp.shear_stiffness, velocity, cp.invMass,
        //     cp.extra_cp_spacing, cp.extra_cp_rad_mult);


        // int cloth_n = int(g_buffers->positions.size());
        

        // Vec3 cloth_upper, cloth_lower;
        // GetParticleBounds(cloth_lower, cloth_upper);
        // //printf("cloth lower: %f %f %f\n", cloth_lower.x, cloth_lower.y, cloth_lower.z);
        // //printf("cloth upper: %f %f %f\n", cloth_upper.x, cloth_upper.y, cloth_upper.z);

        // g_params.radius = cp.particle_radius*1.0f;

        // const int c1 = 0;
        // const int c2 = nx-1;   // <--- [Xiaolvzi]: If use nx, will be a cloth pinned in one corner 

        // g_buffers->positions[c1].w = 0.0f;
        // g_buffers->positions[c2].w = 0.0f;


        // // add tethers
        // for (int i=0; i < int(g_buffers->positions.size()); ++i)
        // {
        //     // hack to rotate cloth
        //     swap(g_buffers->positions[i].y, g_buffers->positions[i].z);
        //     g_buffers->positions[i].y *= -1.0f;

        //     g_buffers->velocities[i] = RandomUnitVector()*0.01f;

        //     float minSqrDist = FLT_MAX;

        //     if (i != c1 && i != c2)
        //     {
        //         float stiffness = -0.8f;
        //         float give = 0.1f;

        //         float sqrDist = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[c2]));

        //         if (sqrDist < minSqrDist)
        //         {
        //             CreateSpring(c1, i, stiffness, give);
        //             CreateSpring(c2, i, stiffness, give);

        //             minSqrDist = sqrDist;
        //         }
        //     }
        // }



        // add object
		//obj_start_index = g_buffers->positions.size();


        // CreateParticleShape(obj,         // mesh
        //     obj_lower,		 			 // lower
        //     1.0f, 						 // scale
        //     0.0f, 					     // rotation
        //     cp.particle_radius,			 // spacing
        //     Vec3(0.0f, -0.0f, 0.0f),     // velocity: red-x; green-y, far away from the ball; blue-z
        //     0.001f,					     // invMass
        //     true,						 // rigid
        //     1.0f, 					     // rigidStiffness
        //     NvFlexMakePhase(group++, 0), // phase
        //     true, 					     // skin
        //     0.0f, 						 // jitter
        //     0.0f, 						 // skinOffset
        //     -cp.particle_radius*1.5f);   // skinExpand






        // add object
        // CreateParticleGrid(Vec3(0.0f, 0.0f, 0.0f), 
        //     16, 16, 16, 0.05f, 
        //     Vec3(0.0f), 1.0f, false, 0.0f, 
        //     NvFlexMakePhase(group++, 0));   



        // CreateParticleSphere(Vec3(0.0f, 0.0f, 0.0f), 
        //     16,
        //     0.05f,
        //     Vec3(0.0f),
        //     0.1f,
        //     true,
        //     1.0f,
        //     NvFlexMakePhase(group++, 0));




        //g_params.radius = radius*1.0f;
        g_params.dynamicFriction = 0.25f;
        g_params.dissipation = 0.0f;
        g_params.numIterations = 4;
        g_params.drag = 0.06f;
        g_params.relaxationFactor = 1.0f;
        g_params.drag = 0.05f;
        g_params.numIterations = 8;
        g_params.dynamicFriction = 0.1625f;
        g_params.collisionDistance = cp.particle_radius;
        g_params.shapeCollisionMargin = cp.particle_radius*0.1f;
        g_params.relaxationFactor = 1.3f;
        g_params.staticFriction = 0.18f; 
        g_drawPoints = false;
        g_drawSprings = false;
    }


    void Destroy() {
        delete obj;
    }


    void Export(const char* basename) {

        char clothPath[400];
        if (g_saveClothPerSimStep)
        {
            sprintf(clothPath, "%s_cloth_%d.obj", basename, g_frame);
        }
        else
        {
            sprintf(clothPath, "%s_cloth.obj", basename);
        }



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
        for (int i = 0; i < obj_start_index; i++)
        {
            sprintf(line, "v %f %f %f\n", particles[i].x, particles[i].y, particles[i].z);
            obj_file << line;
        }


        auto& tris = g_buffers->triangles;
        obj_file << "\n# Faces\n";
        for (int i = 0; i < tris.size()/3; i++)
        {
            sprintf(line, "f %d %d %d\n", tris[i*3+0]+1, tris[i*3+1]+1, tris[i*3+2]+1);
            obj_file << line;
        }


        obj_file << "# Vertices\n";
        obj_file << "o object\n";

        for (int i = obj_start_index; i < g_buffers->positions.size(); i++)
        {
            sprintf(line, "v %f %f %f\n", particles[i].x, particles[i].y, particles[i].z); 
            obj_file << line;
        }    


		int obj_numFaces = int(obj->GetNumFaces());
		obj_file << "\n# Faces\n";

		for (int i=0; i< obj_numFaces; ++i) {
			sprintf(line, "f %d %d %d\n", obj->m_indices[i*3]+1, obj->m_indices[i*3+1]+1, obj->m_indices[i*3+2]+1);
			obj_file << line;
		}
        obj_file.close();



        // char meshPath[300];
        // sprintf(meshPath, "%s_mesh.obj", basename);
        // ExportToObj(&meshPath[0], *obj);


    }



    void Update() {

        // mTime += g_dt;    // <---- 1/60 = 0.16667
        
        // float startTime = 0.0f; // <----- let cloth settle on object
        // float time = Max(0.0f, mTime-startTime);
        // float lastTime = Max(0.0f, time-g_dt);

        // const float rotationSpeed = 1.0f;
        // const float translationSpeed = 1.0f;   // <----- [Important!!!] This value has to be small, otherwise it will cause errors...

        // Vec3 pos = Vec3(obj_center.x, obj_center.z, translationSpeed*time);   //<--- This is in (x, z, y)
        // Vec3 prevPos = Vec3(obj_center.x, obj_center.z, translationSpeed*lastTime);

        // // Quat rot = QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), kPi*(1.0f-cosf(rotationSpeed*time)));
        // // Quat prevRot = QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), kPi*(1.0f-cosf(rotationSpeed*lastTime)));

        // // // AddSphere(0.5f, pos, rot);
        // // //AddCapsule(0.25f, 0.5f, pos, rot);

        //  g_buffers->shapePositions[0] = Vec4(pos, 0.0f);
        //  g_buffers->shapePrevPositions[0] = Vec4(prevPos, 0.0f);

        //  UpdateShapes();

        return ;
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
