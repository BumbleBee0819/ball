
class RigidDrop : public Scene
{
public:

	RigidDrop(const char* name, int brickHeight) : Scene(name), mHeight(brickHeight)
	{
	}
	
	virtual void Initialize()
	{
		int sx = mHeight;
		int sy = mHeight;
		int sz = mHeight;

		Vec3 lower(0.0f, 1.5f + g_params.radius*0.25f, 0.0f);

		int dimx = 2;
		int dimy = 1;
		int dimz = 1;

		Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/box.ply").c_str());
		//Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/shapenet/airplane.obj").c_str());

		// create a basic grid
		for (int y=0; y < dimy; ++y)
			for (int z=0; z < dimz; ++z)
				for (int x=0; x < dimx; ++x)

					// Voxelizes mesh, but only single particle? (TODO - make particles out of) 

					CreateParticleShape(
						mesh,
					
						(g_params.radius*0.905f)*Vec3(float(x*sx), float(y*sy), float(z*sz))
						+ (g_params.radius*0.1f)*Vec3(float(x),float(y),float(z)) + lower, // Lower
						
						g_params.radius*0.9f*Vec3(float(sx), float(sy), float(sz)), // scale
						//0.0f, // rotation
						0.3f, // rotation
						g_params.radius*0.9f, // spacing
						Vec3(0.0f), // velocity
						1.0f, //inv mass
						true, // rigid
						1.0f, // rigid stiffness
						NvFlexMakePhase(g_buffers->rigidOffsets.size()+1, 0),
						true, // skin(?)
						0.002f // jitter (?)
					);

		delete mesh;

		// Platform
		//AddPlinth();

		//g_numExtraParticles = 32*1024;		
		g_numSubsteps = 2;
		g_params.numIterations = 8;

		g_params.radius *= 1.0f;
		g_params.dynamicFriction = 0.4f;
		g_params.dissipation = 0.01f;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.sleepThreshold = g_params.radius*0.25f;
		g_params.shockPropagation = 3.f;
		
		g_windStrength = 0.0f;

		// draw options
		g_drawPoints = false;
	}

	virtual void Update()
	{
			
	}

	int mHeight;
};
