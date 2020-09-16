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
		float stretch_stiffness;
		float bend_stiffness;
		float shear_stiffness;
        float extra_cp_spacing;
        float extra_cp_rad_mult;

		ClothParams(int cs=64, float pr=0.05f, float sts=1.0f, float bs=0.8f, float shs=0.5f, float es=0.0f, float erm=1.0f):
			n_particles(cs), particle_radius(pr), stretch_stiffness(sts),
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

	virtual void Initialize()
	{

        PrintProperties(cp);
        PrintProperties(op);

		/// Add object to drape ///
		obj = ImportMesh(GetFilePathByPlatform(objpath).c_str());
		// obj->Normalize(op.scale); // Amir's change: do not normalize the shapes since our shapes are coming from ShapeNet Core and are already Normalized
		// obj->CenterAtOrigin();

		// apply rotations
        if (op.use_quat) {
            auto x = op.rotate.x;
            auto y = op.rotate.y;
            auto z = op.rotate.z;
            auto w = op.rotate.w;
            // auto w = sqrt(1 - (x*x + y*y + z*z));
            obj->Transform(RotationMatrix(Quat(x, y, z, w)));

        } else {
        	/* Amir's edit: commented out the 3 lines below since we want to apply the rotations the same way as Blender does
        	Note that I set the up and forward axes to Y and -Z when exporting and importing shapes from/into Blender
            obj->Transform(RotationMatrix(op.rotate.x, Vec3(1.0f, 0.0f, 0.0f)));
            obj->Transform(RotationMatrix(op.rotate.y, Vec3(0.0f, 1.0f, 0.0f)));
            obj->Transform(RotationMatrix(op.rotate.z, Vec3(0.0f, 0.0f, 1.0f)));
            */

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

		/// Add cloth ///

		// set cloth size (num particles)
		// int nx, ny, nz = 1;

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
		float fall_height = 0.01f;
		//Vec3 obj_center = 0.5f * (obj_upper + obj_lower);
		// Vec3 obj_center = Vec3(0.0f, 0.0f, 0.0f);
		// Vec3 position_offset = Vec3(
		// 	obj_center.x - 0.5 * nx * (cp.extra_cp_rad_mult*cp.particle_radius + cp.extra_cp_spacing),
		// 	fall_height,
		// 	obj_center.z - 0.5 * ny * (cp.extra_cp_rad_mult*cp.particle_radius + cp.extra_cp_spacing)
		// );

		Vec3 position_offset =Vec3(0.0f, 0.0f, -3.0f);

		printf("111111");
		printf("%f \n %f \n %f\n", position_offset.x, position_offset.y, position_offset.z);

		// other per-particle properties
		float velocity = 0.0f;
		float invMass = 1.0f;

		// create particle group for cloth (self-colliding)
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);

		// make cloth
		cloth_base_idx = int(g_buffers->positions.size());
		CreateSpringGrid(position_offset, nx, ny, 1, cp.particle_radius, phase,
			cp.stretch_stiffness, cp.bend_stiffness, cp.shear_stiffness, velocity, invMass,
            cp.extra_cp_spacing, cp.extra_cp_rad_mult);
		


		Vec3 cloth_upper, cloth_lower;
		GetParticleBounds(cloth_lower, cloth_upper);
		printf("cloth lower: %f %f %f\n", cloth_lower.x, cloth_lower.y, cloth_lower.z);
		printf("cloth upper: %f %f %f\n", cloth_upper.x, cloth_upper.y, cloth_upper.z);

		g_params.radius = cp.particle_radius*1.0f;

		const int c1 = 0;
		const int c2 = nx;

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
				

				float stiffness = -0.8f;
				float give = 0.1f;

				float sqrDist = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[c2]));

				if (sqrDist < minSqrDist)
				{
					CreateSpring(c1, i, stiffness, give);
					CreateSpring(c2, i, stiffness, give);

					minSqrDist = sqrDist;
				}
			}
		}


		//g_params.radius = radius*1.0f;
		g_params.dynamicFriction = 0.25f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 4;
		g_params.drag = 0.06f;
		g_params.relaxationFactor = 1.0f;


		
		if (!g_params.drag)
			g_params.drag = 0.05f;

		// g_params.dissipation = 0.0f;
		// g_params.viscosity = 0.0f;

		if (!g_params.numIterations)
			g_params.numIterations = 8;

		if (!g_params.dynamicFriction)
			g_params.dynamicFriction = 0.1625f;

		if (!g_params.collisionDistance)
			g_params.collisionDistance = cp.particle_radius;

		if (!g_params.shapeCollisionMargin)
			g_params.shapeCollisionMargin = cp.particle_radius*0.1f;

		g_params.relaxationFactor = 1.3f;

		g_windStrength = 10.0f;

		g_params.staticFriction = 0.18f; // Amir's edit: Added this line. Default value would be 0.0f is this line is removed

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
		if (g_saveClothPerSimStep)
		{
			//sprintf(clothPath, "%s_cloth_%d.obj", basename, g_frame);
		}
		else
		{
			//sprintf(clothPath, "%s_cloth.obj", basename);
		}


		/*
		Amir's edit
		char meshPath[300];
		sprintf(meshPath, "%s_mesh.obj", basename);
		char contactsPath[300];
		sprintf(contactsPath, "%s_contacts.txt", basename);
		*/



		//printf("Exporting cloth to %s\n", clothPath);

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
		for (int i = 0; i < tris.size()/3; i++)
		{
		    sprintf(line, "f %d %d %d\n", tris[i*3+0]+1, tris[i*3+1]+1, tris[i*3+2]+1);
		    obj_file << line;
		}

		obj_file.close();

		/*
		Amir's edit: we don't need to save the mesh objs. We only need the cloth obj file and the previous lines take care of that
		printf("Exporting mesh to %s\n", meshPath);

		ExportToObj(&meshPath[0], *obj);
		*/
		

		
		/*
		Amir's edit: disable doing unnecessary calculations for contact points
        if (g_exportContacts) {
            printf("Exporting contact points to %s\n", contactsPath);

            auto contacts_stuff = ExtractContactPoints();
            ofstream cts_file(contactsPath);

            if (!cts_file) {
                printf("Failed to write to %s\n", contactsPath);
            } else {
                for (int i : contacts_stuff.first) {
                    sprintf(line, "%f,%f,%f\n", particles[i].x, particles[i].y, particles[i].z);
                    cts_file << line;
                }

                cts_file.close();
            }
        }
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
		// 	int base = 3*i;
		// 	Vec3& n = normals[i];

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



	pair<vector<int>, vector<pair<int,int> > > ExtractContactPoints() {

		auto& cparticles = g_buffers->positions; // cloth particles
        auto& mparticles = obj->m_positions; // mesh particles

        //vector<Vec4> cparticles_to_check;

		//Vec3 m_lower, m_upper;
		//obj->GetBounds(m_lower, m_upper);

       // for (int i = 0; i < int(cparticles.size()); ++i) {
       //     Vec4& cpt = cparticles[i];
       //     if (cpt.y >= m_lower.y) {
       //         cparticles_to_check.push_back(cpt);
       //     }
       // }

        vector<pair<int,int> > contact_pairs; // first = cloth particle (index),
                                              // second = mesh vertex (index)
        vector<int> contact_points; // cloth particles (indices) touching mesh

        double start_t = GetSeconds();


        //for (int i = 0; i < int(cparticles_to_check.size()); ++i) {
        for (int i = 0; i < int(cparticles.size()); ++i) {
            Vec4& cpt = cparticles[i];
            bool added = false;
            for (int j = 0; j < int(mparticles.size()); ++j) {
                auto& mpt = mparticles[j];
                auto dx2 = Sqr(cpt.x - mpt.x);
                auto dy2 = Sqr(cpt.y - mpt.y);
                auto dz2 = Sqr(cpt.z - mpt.z);

                if (Sqrt(dx2 + dy2 + dz2) <= contact_eps) {
                    contact_pairs.push_back({i, j});
                    if (!added) {
                        contact_points.push_back(i);
                        added = true;
                    }
                }
            }
        }

        double end_t = GetSeconds();

        //int cptc_sz = cparticles_to_check.size();

        printf("Contact distance: <=%f\n", contact_eps);
        //printf("Number of checked points: %d (%f%% of cloth)\n",
        //    cptc_sz, float(cptc_sz)/float(cparticles.size()) * 100);
        printf("Number of contact points: %d\n", int(contact_points.size()));
        printf("Number of contact pairs: %d\n", int(contact_pairs.size()));
        printf("Time elapsed for n^2 check: %f s\n", end_t - start_t);

        return { contact_points, contact_pairs };

	}







private:

	const char* objpath;

	ClothParams cp;
	ObjParams op;

	int cloth_base_idx;
	int nx, ny;

    float contact_eps; // max distance for cloth-mesh "contact"

	// Returns cloth dimensions to fully drape mesh object in all radial
	// 	directions, i.e. drape the smallest enclosing sphere
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
