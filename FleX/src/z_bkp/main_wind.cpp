#include "../core/types.h"
#include "../core/maths.h"
#include "../core/platform.h"
#include "../core/mesh.h"
#include "../core/voxelize.h"
#include "../core/sdf.h"
#include "../core/pfm.h"
#include "../core/tga.h"
#include "../core/perlin.h"
#include "../core/convex.h"
#include "../core/cloth.h"

#include "../external/SDL2-2.0.4/include/SDL.h"
#include <yaml-cpp/yaml.h>

#include "../include/NvFlex.h"
#include "../include/NvFlexExt.h"
#include "../include/NvFlexDevice.h"

#include <iostream>
#include <fstream>
#include <map>
#include <random>

#include "shaders.h"
#include "imgui.h"

using namespace std;

#include "globals.h"

void DrawShapes();

inline float sqr(float x) { return x*x; }

#include "helpers.h"
#include "scenes.h"
#include "benchmark.h"
#include "controller.h"

void ErrorCallback(NvFlexErrorSeverity severity, const char* msg, const char* file, int line)
{
	printf("Flex: %s - %s:%d\n", msg, file, line);
	g_Error = (severity == eNvFlexLogError);
}

void PrintSimParams() {
	printf("\n ---- Simulation Params ----\n");
    printf("substeps:                  %d\n", g_numSubsteps);
    printf("radius:                    %f\n", g_params.radius);
    printf("num_planes:                %d\n", g_params.numPlanes);
    printf("iterations:                %d\n", g_params.numIterations);
    printf("restitution:               %f\n", g_params.restitution);
    printf("dissipation:               %f\n", g_params.dissipation);
    printf("damping:                   %f\n", g_params.damping);
    printf("drag:                      %f\n", g_params.drag);
    printf("lift:                      %f\n", g_params.lift);
    printf("dynamic_friction:          %f\n", g_params.dynamicFriction);
    printf("static_friction:           %f\n", g_params.staticFriction);
    printf("particle_friction:         %f\n", g_params.particleFriction);
    printf("collision_distance:        %f\n", g_params.collisionDistance);
    printf("shape_collision_margin:    %f\n", g_params.shapeCollisionMargin);
    printf("particle_collision_margin: %f\n", g_params.particleCollisionMargin);
    printf("relaxation_factor:         %f\n", g_params.relaxationFactor);
    printf("relaxation_mode:           %d\n", g_params.relaxationMode);
    printf("viscosity:                 %f\n", g_params.viscosity);
    printf("light_distance:            %f\n", g_lightDistance);
    printf("saveClothPerSimStep:       %d\n", g_saveClothPerSimStep);
    printf("\n");
}

void ParseConfig(const char* path, int& n_iters, Wind::ClothParams& cp, Wind::ObjParams& op) {
	typedef YAML::Node Node;
	using YAML::LoadFile;

	Node config;
	int randSeedNum;

	try {
		config = LoadFile(path);
	} catch (YAML::BadFile) {
		cerr << "Error: Improperly formatted YAML config." << endl;
		exit(-1);
	}

	// global
    if (config["steps"]) {
        n_iters = config["steps"].as<int>();
    }

    g_numSubsteps = config["substeps"].as<int>(g_numSubsteps);
    g_params.numIterations = config["subiters"].as<int>(g_params.numIterations);

    g_params.restitution = config["restitution"].as<float>(g_params.restitution);
    g_params.dissipation = config["dissipation"].as<float>(g_params.dissipation);
    g_params.damping = config["damping"].as<float>(g_params.damping);

    if (g_clothDrag != -1)
    {
    	g_params.drag = g_clothDrag;
    }
    else
    {
	    g_params.drag = config["drag"].as<float>(g_params.drag);
	}

    if (g_clothLift != -1)
    {
    	g_params.lift = g_clothLift;
    }
    else
    {
	    g_params.lift = config["lift"].as<float>(g_params.lift);
	}

	if (g_clothFriction != -1)
    {
    	g_params.dynamicFriction = g_clothFriction;
    }
    else
    {
	    g_params.dynamicFriction = config["friction"].as<float>(g_params.dynamicFriction);
	}

    
    g_params.dynamicFriction = config["dynamic_friction"].as<float>(g_params.dynamicFriction);
    g_params.staticFriction = config["static_friction"].as<float>(g_params.staticFriction);
    g_params.particleFriction = config["particle_friction"].as<float>(g_params.particleFriction);

    if (config["friction"] && config["dynamic_friction"]) {
    	printf("Ignoring \"friction\" in config file (same as \"dynamic_friction\").\n");
    }

    g_params.collisionDistance = config["coll_distance"].as<float>(g_params.collisionDistance);
    g_params.shapeCollisionMargin = config["coll_margin"].as<float>(g_params.shapeCollisionMargin);
    g_params.shapeCollisionMargin = config["shape_coll_margin"].as<float>(g_params.shapeCollisionMargin);
    g_params.particleCollisionMargin = config["particle_coll_margin"].as<float>(g_params.particleCollisionMargin);

    if (config["coll_margin"] && config["shape_coll_margin"]) {
    	printf("Ignoring \"coll_margin\" (same as \"shape_coll_friction\").\n");
    }

    if (config["extra_cp_spacing"]) {
        cp.extra_cp_spacing = config["extra_cp_spacing"].as<float>(cp.extra_cp_spacing);
    }

    if (config["extra_cp_rad_mult"]) {
        cp.extra_cp_rad_mult = config["extra_cp_rad_mult"].as<float>(cp.extra_cp_rad_mult);
    }


    if (config["bend_stiffness"] || config["stretch_stiffness"] || config["shear_stiffness"]) {
    	if (config["stiffness"]) {
    		printf("Ignoring \"stiffness\" (overriden by \"{bend, stretch, shear}_stiffness\".\n");
    	}

		cp.bend_stiffness = config["bend_stiffness"].as<float>(cp.bend_stiffness);
		cp.shear_stiffness = config["shear_stiffness"].as<float>(cp.shear_stiffness);
		cp.stretch_stiffness = config["stretch_stiffness"].as<float>(cp.stretch_stiffness);

    } else {

    	float stiff_scale;
	    if (g_clothStiffness <= 0.0)
	    {
	    	stiff_scale = 1.0 + (rand() / (RAND_MAX/(0.6-1.0))); // Low = 0.6, High = 1.0
	    	random_device rd;
			randSeedNum = rd();
			mt19937 gen(randSeedNum);
		    uniform_int_distribution<> dis(0, 5);
	    	g_params.dissipation = dis(gen);
	    }
	    else if (g_clothStiffness != -1)
	    {
	    	stiff_scale = g_clothStiffness;
	    }
	    else
	    {
	    	stiff_scale = config["stiffness"].as<float>(1.0f);
	    }

		cp.bend_stiffness *= stiff_scale;
		cp.shear_stiffness *= stiff_scale;
		cp.stretch_stiffness *= stiff_scale;
    }

    // Cloth particles properties
    // if ((g_particleRadius == 0) & (cp.n_particles != 0))
    // {
    // 	cp.particle_radius = config["particle_size"].as<float>(cp.particle_radius);
    // }
    // else
    // {
    // 	cp.particle_radius = g_particleRadius;
    // }

    if (cp.particle_radius == 0)
    {
	    cp.particle_radius = g_particleRadius;
	}
	if (cp.n_particles <= 0)
	{
		if (g_randomSeed != -1)
		{
			randSeedNum = g_randomSeed;
		}
		else
		{
			random_device rd;
			randSeedNum = rd();
		}
		mt19937 gen(randSeedNum);
	    uniform_int_distribution<> dis(g_randomClothMinRes, g_randomClothMaxRes);
		cp.n_particles = dis(gen);
		cp.particle_radius = cp.particle_radius * (1.0+(1.0-(float(pow(cp.n_particles, 2.2))/float(pow(g_clothNumParticles, 2.2)))));
	}
	else
	{
		cp.n_particles = config["cloth_size"].as<float>(cp.n_particles);
	}

	// camera
	g_fixCamera = config["fix_cam"].as<bool>(g_fixCamera);
	g_camLateralAngle = config["cam_angle"].as<float>(g_camLateralAngle);
	g_camDownTilt = config["cam_tilt"].as<float>(g_camDownTilt);
	g_cam_r = config["cam_dist"].as<float>(g_cam_r);
	g_camOffset.x = config["cam_offset_x"].as<float>(g_camOffset.x);
	g_camOffset.z = config["cam_offset_z"].as<float>(g_camOffset.z);
	g_camOffset.y = config["cam_offset_y"].as<float>(g_camOffset.y);

	// object
	/*
	Amir's Edit: We assume the numbers already come in radians, if usign Euler angles
    if(config["rx"]) {
	    op.rotate.x = DegToRad(config["rx"].as<float>());
    }
    if(config["ry"]) {
	    op.rotate.y = DegToRad(config["ry"].as<float>());
    }
    if(config["rz"]) {
	    op.rotate.z = DegToRad(config["rz"].as<float>());
    }
    */
}

void SDLInit(const char* title)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)	// Initialize SDL's Video subsystem and game controllers
		printf("Unable to initialize SDL");

	unsigned int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL; // <- tell SDL to use OpenGL context

	g_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		g_screenWidth, g_screenHeight, flags);

	g_windowId = SDL_GetWindowID(g_window);
}

void InitRender() {
	const char* title = "Flex Demo (CUDA)";
	SDLInit(title);

	RenderInitOptions options;
	options.window = g_window;
	options.numMsaaSamples = g_msaaSamples;
	options.asyncComputeBenchmark = g_asyncComputeBenchmark;
	options.defaultFontHeight = -1;
	options.fullscreen = g_fullscreen;

	InitGL(options);

	if (g_fullscreen)
		SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	ReshapeWindow(g_screenWidth, g_screenHeight);

	// create shadow maps
	g_shadowMap = ShadowCreate();
}


void InitSim() {            // <---- Initialize the solver. This is the first step.
	cout<<"InitSim"<<endl;
	NvFlexInitDesc desc;
	desc.deviceIndex = g_device;
	desc.enableExtensions = g_extensions;
	desc.renderDevice = 0;
	desc.renderContext = 0;
	desc.computeContext = 0;
	desc.computeType = eNvFlexCUDA;

	// Init Flex library, note that no CUDA methods should be called before this 
	// point to ensure we get the device context we want
	g_flexLib = NvFlexInit(NV_FLEX_VERSION, ErrorCallback, &desc);

	if (g_Error || g_flexLib == NULL)
	{
		printf("Could not initialize Flex, exiting.\n");
		exit(-1);
	}

	// store device name
	strcpy(g_deviceName, NvFlexGetDeviceName(g_flexLib));
}

void ResetVisGlobals() {    // <----- define g_* parameters
	cout<<"ResetVisGlobals"<<endl;
	g_blur = 1.0f;
	g_fluidColor = Vec4(0.1f, 0.4f, 0.8f, 1.0f);
	g_meshColor = Vec3(0.9f, 0.9f, 0.9f);
	g_drawEllipsoids = false;
	g_drawPoints = true;
	g_drawCloth = true;
	g_expandCloth = 0.0f;

	g_diffuseScale = 0.5f;
	g_diffuseColor = 1.0f;
	g_diffuseMotionScale = 1.0f;
	g_diffuseShadow = false;
	g_diffuseInscatter = 0.8f;
	g_diffuseOutscatter = 0.53f;

	// reset phase 0 particle color to blue
	g_colors[0] = Colour(0.0f, 0.5f, 1.0f);

	g_mouseParticle = -1;
}

void ResetSimGlobals() {

    cout<<"ResetSimGlobals"<<endl;

	if (g_buffers)
		DestroyBuffers(g_buffers);

	// alloc buffers
	g_buffers = AllocBuffers(g_flexLib);

	// map during initialization
	MapBuffers(g_buffers);

	g_buffers->positions.resize(0);
	g_buffers->velocities.resize(0);
	g_buffers->phases.resize(0);

	g_buffers->rigidOffsets.resize(0);
	g_buffers->rigidIndices.resize(0);
	g_buffers->rigidMeshSize.resize(0);
	g_buffers->rigidRotations.resize(0);
	g_buffers->rigidTranslations.resize(0);
	g_buffers->rigidCoefficients.resize(0);
	g_buffers->rigidPlasticThresholds.resize(0);
	g_buffers->rigidPlasticCreeps.resize(0);
	g_buffers->rigidLocalPositions.resize(0);
	g_buffers->rigidLocalNormals.resize(0);

	g_buffers->springIndices.resize(0);
	g_buffers->springLengths.resize(0);
	g_buffers->springStiffness.resize(0);
	g_buffers->triangles.resize(0);
	g_buffers->triangleNormals.resize(0);
	g_buffers->uvs.resize(0);

	g_meshSkinIndices.resize(0);
	g_meshSkinWeights.resize(0);

	g_buffers->shapeGeometry.resize(0);
	g_buffers->shapePositions.resize(0);
	g_buffers->shapeRotations.resize(0);
	g_buffers->shapePrevPositions.resize(0);
	g_buffers->shapePrevRotations.resize(0);
	g_buffers->shapeFlags.resize(0);

	delete g_mesh; g_mesh = NULL;

	g_frame = 0;
	g_pause = false;

	g_dt = 1.0f / 60.0f;
	g_waveTime = 0.0f;
	g_windTime = 0.0f;
	g_windStrength = 0.0f;
	g_windFrequency = 0.1*(1.0f/g_dt);  //wind by Xiaolvzi

	g_drawOpaque = false;
	g_drawSprings = false;
	g_drawDiffuse = false;
	g_drawMesh = true;
	g_drawDensity = false;
	g_ior = 1.0f;
	g_lightDistance = 2.0f;
	g_fogDistance = 0.005f;

	g_camSpeed = 0.075f;
	g_camNear = 0.01f;
	g_camFar = 1000.0f;

	g_pointScale = 1.0f;
	g_drawPlaneBias = 0.0f;

	// sim params
	g_params.gravity[0] = 0.0f;
	g_params.gravity[1] = -9.8f;
	g_params.gravity[2] = 0.0f;

	g_params.wind[0] = 0.0f;
	g_params.wind[1] = 0.0f;
	g_params.wind[2] = 0.0f;

	g_params.radius = 0.15f;
	g_params.viscosity = 0.0f;
	// g_params.dynamicFriction = 0.0f;
	// g_params.staticFriction = 0.0f;
	// g_params.particleFriction = 0.0f; // scale friction between particles by default
	g_params.freeSurfaceDrag = 0.0f;
	// g_params.drag = 0.0f;
	// g_params.lift = 0.0f;
	//g_params.numIterations = 3;
	g_params.fluidRestDistance = 0.0f;
	g_params.solidRestDistance = 0.0f;

	g_params.anisotropyScale = 1.0f;
	g_params.anisotropyMin = 0.1f;
	g_params.anisotropyMax = 2.0f;
	g_params.smoothing = 1.0f;

	// g_params.dissipation = 0.0f;
	//g_params.damping = 0.0f;
	// g_params.particleCollisionMargin = 0.0f;
	//g_params.shapeCollisionMargin = 0.0f;
	//g_params.collisionDistance = 0.0f;
	g_params.sleepThreshold = 0.0f;
	g_params.shockPropagation = 0.0f;
	// g_params.restitution = 0.0f;

	g_params.maxSpeed = FLT_MAX;
	g_params.maxAcceleration = 100.0f;	// approximately 10x gravity

	g_params.relaxationMode = eNvFlexRelaxationLocal;
	g_params.relaxationFactor = 1.0f;
	g_params.solidPressure = 1.0f;
	g_params.adhesion = 0.0f;
	g_params.cohesion = 0.025f;
	g_params.surfaceTension = 0.0f;
	g_params.vorticityConfinement = 0.0f;
	g_params.buoyancy = 1.0f;
	g_params.diffuseThreshold = 100.0f;
	g_params.diffuseBuoyancy = 1.0f;
	g_params.diffuseDrag = 0.8f;
	g_params.diffuseBallistic = 16;
	g_params.diffuseLifetime = 2.0f;

	// planes created after particles
	if (g_hasGround) {
		g_params.numPlanes = 1;
	} else {
		g_params.numPlanes = 0;
	}

	g_numSolidParticles = 0;

	g_waveFrequency = 1.5f;
	g_waveAmplitude = 1.5f;
	g_waveFloorTilt = 0.0f;

	g_emit = false;
	g_warmup = false;

	g_maxDiffuseParticles = 0;	// number of diffuse particles
	g_maxNeighborsPerParticle = 96;
	g_numExtraParticles = 0;	// number of particles allocated but not made active

	g_sceneLower = FLT_MAX;
	g_sceneUpper = -FLT_MAX;
}

// Angle and tilt should be in degrees
void OrientCamera(float angle=0.0f, float tilt=0.0f) {
	cout<<"OrientCamera"<<endl;
	

	Vec3 particleLower, particleUpper;
	GetParticleBounds(particleLower, particleUpper);

	// printf("Particle Bounds: [%f, %f], [%f, %f], [%f, %f]\n",
	// 	particleLower.x, particleUpper.x,
	// 	particleLower.y, particleUpper.y,
	// 	particleLower.z, particleUpper.z);

	Vec3 sceneLower = Min(particleLower, g_shapeLower);
	Vec3 sceneUpper = Max(particleUpper, g_shapeUpper);
	Vec3 sceneCenter = (sceneUpper + sceneLower) * 0.5f;
	//Vec3 sceneExtents = sceneUpper - sceneLower;

	// printf("Scene Bounds: [%f, %f], [%f, %f], [%f, %f]\n",
	// 	sceneLower.x, sceneUpper.x,
	// 	sceneLower.y, sceneUpper.y,
	// 	sceneLower.z, sceneUpper.z);

	// constexpr float cam_dist_to_scene_bbox = 2.0f / 1.0f;
	// const float cam_r = cam_dist_to_scene_bbox * Max(sceneExtents.x, sceneExtents.y);
	// constexpr float cam_r = 6.0f; // distance from origin on xz-plane
	constexpr float extra_tilt_factor = 1.0f; // tilt a bit more to center the cloth blob	

	tilt = DegToRad(tilt);
	angle = DegToRad(angle);

	float x = sceneCenter.x + g_cam_r * sinf(angle) + g_camOffset.x;
	float z = sceneCenter.z + g_cam_r * cosf(angle) + g_camOffset.z;
	float y = sceneCenter.y + g_cam_r * tanf(tilt)  + g_camOffset.y;

	g_camPos = Vec3(x, y, z);
	g_camAngle = Vec3(angle, -tilt * extra_tilt_factor, 0.0f);
}

void ResetRender(bool centerCamera) {
	cout<<"ResetRender"<<endl;

	if(g_fluidRenderBuffers)
		DestroyFluidRenderBuffers(g_fluidRenderBuffers);
	if(g_diffuseRenderBuffers)
		DestroyDiffuseRenderBuffers(g_diffuseRenderBuffers);

	// center camera on particles
	OrientCamera(g_camLateralAngle, g_camDownTilt);
	g_scenes[g_scene]->CenterCamera();
	// if (centerCamera)
	// {
		// g_camPos = Vec3((g_sceneLower.x + g_sceneUpper.x) * 0.5f, g_sceneUpper.y * 1.25f, g_sceneUpper.z + g_sceneUpper.y * 2.0f);
		//g_camPos = Vec3((g_sceneLower.x + g_sceneUpper.x) * 0.5f, g_sceneUpper.y * 1.25f, g_sceneUpper.z);
		// g_camAngle = Vec3(0.0f, -DegToRad(15.0f), 0.0f);

		// give scene a chance to modify camera position
	// }

	g_fluidRenderBuffers = CreateFluidRenderBuffers(g_solverDesc.maxParticles, g_interop);
	g_diffuseRenderBuffers = CreateDiffuseRenderBuffers(g_maxDiffuseParticles, g_interop);
}

void ResetSim() {
	cout<<"ResetSim"<<endl;

	if(g_solver) {
		NvFlexDestroySolver(g_solver);
		g_solver = NULL;
	}

	// initialize solver desc
	NvFlexSetSolverDescDefaults(&g_solverDesc);

	// create scene
	StartGpuWork();
	g_scenes[g_scene]->Initialize();
	EndGpuWork();

	uint32_t numParticles = g_buffers->positions.size();
	uint32_t maxParticles = numParticles + g_numExtraParticles * g_numExtraMultiplier;

	g_solverDesc.maxParticles = maxParticles;
	g_solverDesc.maxDiffuseParticles = g_maxDiffuseParticles;
	g_solverDesc.maxNeighborsPerParticle = g_maxNeighborsPerParticle;

	// main create method for the Flex solver
	g_solver = NvFlexCreateSolver(g_flexLib, &g_solverDesc);
	
	if (g_params.solidRestDistance == 0.0f)
		g_params.solidRestDistance = g_params.radius;

	// if fluid present then we assume solid particles have the same radius
	if (g_params.fluidRestDistance > 0.0f)
		g_params.solidRestDistance = g_params.fluidRestDistance;

	// set collision distance automatically based on rest distance if not alraedy set
	if (g_params.collisionDistance == 0.0f)
		g_params.collisionDistance = Max(g_params.solidRestDistance, g_params.fluidRestDistance)*0.5f;

	// default particle friction to 10% of shape friction
	if (g_params.particleFriction == 0.0f)
		g_params.particleFriction = g_params.dynamicFriction*0.1f;

	// add a margin for detecting contacts between particles and shapes
	if (g_params.shapeCollisionMargin == 0.0f)
		g_params.shapeCollisionMargin = g_params.collisionDistance*0.5f;

	// calculate particle bounds
	Vec3 particleLower, particleUpper;
	GetParticleBounds(particleLower, particleUpper);

	// accommodate shapes
	//Vec3 shapeLower, shapeUpper;
	GetShapeBounds(g_shapeLower, g_shapeUpper);

	// printf("Particle Bounds: [%f, %f], [%f, %f], [%f, %f]\n",
	// 	particleLower.x, particleUpper.x,
	// 	particleLower.y, particleUpper.y,
	// 	particleLower.z, particleUpper.z);

	// printf("Shape Bounds: [%f, %f], [%f, %f], [%f, %f]\n",
	// 	g_shapeLower.x, g_shapeUpper.x,
	// 	g_shapeLower.y, g_shapeUpper.y,
	// 	g_shapeLower.z, g_shapeUpper.z);

	// update bounds
	g_sceneLower = Min(Min(g_sceneLower, particleLower), g_shapeLower);
	g_sceneUpper = Max(Max(g_sceneUpper, particleUpper), g_shapeUpper);

	g_sceneLower -= g_params.collisionDistance;
	g_sceneUpper += g_params.collisionDistance;

	// printf("Scene Bounds: [%f, %f], [%f, %f], [%f, %f]\n",
	// 	g_sceneLower.x, g_sceneUpper.x,
	// 	g_sceneLower.y, g_sceneUpper.y,
	// 	g_sceneLower.z, g_sceneUpper.z);

	// update collision planes to match flexs
	Vec3 up = Normalize(Vec3(-g_waveFloorTilt, 1.0f, 0.0f));

	(Vec4&)g_params.planes[0] = Vec4(up.x, up.y, up.z, 0.0f);
	(Vec4&)g_params.planes[1] = Vec4(0.0f, 0.0f, 1.0f, -g_sceneLower.z);
	(Vec4&)g_params.planes[2] = Vec4(1.0f, 0.0f, 0.0f, -g_sceneLower.x);
	(Vec4&)g_params.planes[3] = Vec4(-1.0f, 0.0f, 0.0f, g_sceneUpper.x);
	(Vec4&)g_params.planes[4] = Vec4(0.0f, 0.0f, -1.0f, g_sceneUpper.z);
	(Vec4&)g_params.planes[5] = Vec4(0.0f, -1.0f, 0.0f, g_sceneUpper.y);

	g_wavePlane = g_params.planes[2][3];

	g_buffers->diffusePositions.resize(g_maxDiffuseParticles);
	g_buffers->diffuseVelocities.resize(g_maxDiffuseParticles);
	g_buffers->diffuseCount.resize(1, 0);

	// for fluid rendering these are the Laplacian smoothed positions
	g_buffers->smoothPositions.resize(maxParticles);

	// initialize normals (just for rendering before simulation starts)
	g_buffers->normals.resize(0);
	g_buffers->normals.resize(maxParticles);

	int numTris = g_buffers->triangles.size() / 3;
	for (int i = 0; i < numTris; ++i)
	{
		Vec3 v0 = Vec3(g_buffers->positions[g_buffers->triangles[i * 3 + 0]]);
		Vec3 v1 = Vec3(g_buffers->positions[g_buffers->triangles[i * 3 + 1]]);
		Vec3 v2 = Vec3(g_buffers->positions[g_buffers->triangles[i * 3 + 2]]);

		Vec3 n = Cross(v1 - v0, v2 - v0);

		g_buffers->normals[g_buffers->triangles[i * 3 + 0]] += Vec4(n, 0.0f);
		g_buffers->normals[g_buffers->triangles[i * 3 + 1]] += Vec4(n, 0.0f);
		g_buffers->normals[g_buffers->triangles[i * 3 + 2]] += Vec4(n, 0.0f);
	}

	for (int i = 0; i < int(maxParticles); ++i)
		g_buffers->normals[i] = Vec4(SafeNormalize(Vec3(g_buffers->normals[i]), Vec3(0.0f, 1.0f, 0.0f)), 0.0f);

	// save mesh positions for skinning
	// NOTE: but g_mesh = NULL after ResetSimGlobals()
	if (g_mesh)
	{
		g_meshRestPositions = g_mesh->m_positions;
	}
	else
	{
		g_meshRestPositions.resize(0);
	}

	// give scene a chance to do some post solver initialization
	g_scenes[g_scene]->PostInitialize();

	// create active indices (just a contiguous block for the demo)
	g_buffers->activeIndices.resize(g_buffers->positions.size());
	for (int i = 0; i < g_buffers->activeIndices.size(); ++i)
		g_buffers->activeIndices[i] = i;

	// resize particle buffers to fit
	g_buffers->positions.resize(maxParticles);
	g_buffers->velocities.resize(maxParticles);
	g_buffers->phases.resize(maxParticles);

	g_buffers->densities.resize(maxParticles);
	g_buffers->anisotropy1.resize(maxParticles);
	g_buffers->anisotropy2.resize(maxParticles);
	g_buffers->anisotropy3.resize(maxParticles);

	// save rest positions
	g_buffers->restPositions.resize(g_buffers->positions.size());
	for (int i = 0; i < g_buffers->positions.size(); ++i)
		g_buffers->restPositions[i] = g_buffers->positions[i];

	// builds rigids constraints
	if (g_buffers->rigidOffsets.size())
	{
		assert(g_buffers->rigidOffsets.size() > 1);

		const int numRigids = g_buffers->rigidOffsets.size() - 1;

		// If the centers of mass for the rigids are not yet computed, this is done here
		// (If the CreateParticleShape method is used instead of the NvFlexExt methods, the centers of mass will be calculated here)
		if (g_buffers->rigidTranslations.size() == 0) 
		{
			g_buffers->rigidTranslations.resize(g_buffers->rigidOffsets.size() - 1, Vec3());
			CalculateRigidCentersOfMass(&g_buffers->positions[0], g_buffers->positions.size(), &g_buffers->rigidOffsets[0], &g_buffers->rigidTranslations[0], &g_buffers->rigidIndices[0], numRigids);
		}

		// calculate local rest space positions
		g_buffers->rigidLocalPositions.resize(g_buffers->rigidOffsets.back());
		CalculateRigidLocalPositions(&g_buffers->positions[0], &g_buffers->rigidOffsets[0], &g_buffers->rigidTranslations[0], &g_buffers->rigidIndices[0], numRigids, &g_buffers->rigidLocalPositions[0]);

		// set rigidRotations to correct length, probably NULL up until here
		g_buffers->rigidRotations.resize(g_buffers->rigidOffsets.size() - 1, Quat());
	}

	PrintSimParams();

	// unmap so we can start transferring data to GPU
	UnmapBuffers(g_buffers);

	//-----------------------------
	// Send data to Flex

	NvFlexCopyDesc copyDesc;
	copyDesc.dstOffset = 0;
	copyDesc.srcOffset = 0;
	copyDesc.elementCount = numParticles;

	NvFlexSetParams(g_solver, &g_params);
	NvFlexSetParticles(g_solver, g_buffers->positions.buffer, &copyDesc);
	NvFlexSetVelocities(g_solver, g_buffers->velocities.buffer, &copyDesc);
	NvFlexSetNormals(g_solver, g_buffers->normals.buffer, &copyDesc);
	NvFlexSetPhases(g_solver, g_buffers->phases.buffer, &copyDesc);
	NvFlexSetRestParticles(g_solver, g_buffers->restPositions.buffer, &copyDesc);

	NvFlexSetActive(g_solver, g_buffers->activeIndices.buffer, &copyDesc);
	NvFlexSetActiveCount(g_solver, numParticles);
	
	// springs
	if (g_buffers->springIndices.size())
	{
		assert((g_buffers->springIndices.size() & 1) == 0);
		assert((g_buffers->springIndices.size() / 2) == g_buffers->springLengths.size());

		NvFlexSetSprings(g_solver, g_buffers->springIndices.buffer, g_buffers->springLengths.buffer, g_buffers->springStiffness.buffer, g_buffers->springLengths.size());
	}

	// rigids
	if (g_buffers->rigidOffsets.size())
	{
		NvFlexSetRigids(g_solver, g_buffers->rigidOffsets.buffer, g_buffers->rigidIndices.buffer, g_buffers->rigidLocalPositions.buffer, g_buffers->rigidLocalNormals.buffer, g_buffers->rigidCoefficients.buffer, g_buffers->rigidPlasticThresholds.buffer, g_buffers->rigidPlasticCreeps.buffer, g_buffers->rigidRotations.buffer, g_buffers->rigidTranslations.buffer, g_buffers->rigidOffsets.size() - 1, g_buffers->rigidIndices.size());
	}

	// inflatables
	if (g_buffers->inflatableTriOffsets.size())
	{
		NvFlexSetInflatables(g_solver, g_buffers->inflatableTriOffsets.buffer, g_buffers->inflatableTriCounts.buffer, g_buffers->inflatableVolumes.buffer, g_buffers->inflatablePressures.buffer, g_buffers->inflatableCoefficients.buffer, g_buffers->inflatableTriOffsets.size());
	}

	// dynamic triangles
	if (g_buffers->triangles.size())
	{
		NvFlexSetDynamicTriangles(g_solver, g_buffers->triangles.buffer, g_buffers->triangleNormals.buffer, g_buffers->triangles.size() / 3);
	}

	// collision shapes
	if (g_buffers->shapeFlags.size())
	{
		NvFlexSetShapes(
			g_solver,
			g_buffers->shapeGeometry.buffer,
			g_buffers->shapePositions.buffer,
			g_buffers->shapeRotations.buffer,
			g_buffers->shapePrevPositions.buffer,
			g_buffers->shapePrevRotations.buffer,
			g_buffers->shapeFlags.buffer,
			int(g_buffers->shapeFlags.size()));
	}

	// perform initial sim warm up
	if (g_warmup)
	{
		printf("Warming up sim..\n");

		// warm it up (relax positions to reach rest density without affecting velocity)
		NvFlexParams copy = g_params;
		copy.numIterations = 4;

		NvFlexSetParams(g_solver, &copy);

		const int kWarmupIterations = 100;

		for (int i = 0; i < kWarmupIterations; ++i)
		{
			NvFlexUpdateSolver(g_solver, 0.0001f, 1, false);
			NvFlexSetVelocities(g_solver, g_buffers->velocities.buffer, NULL);
		}

		// udpate host copy
		NvFlexGetParticles(g_solver, g_buffers->positions.buffer, NULL);
		NvFlexGetSmoothParticles(g_solver, g_buffers->smoothPositions.buffer, NULL);
		NvFlexGetAnisotropy(g_solver, g_buffers->anisotropy1.buffer, g_buffers->anisotropy2.buffer, g_buffers->anisotropy3.buffer, NULL);

		printf("Finished warm up.\n");
	}

}

void ResetCoupledAssets() {

	cout<<"ResetCoupledAssets"<<endl;
	for (auto& iter : g_meshes)
	{
		NvFlexDestroyTriangleMesh(g_flexLib, iter.first);
		DestroyGpuMesh(iter.second); // OpenGL
	}

	for (auto& iter : g_fields)
	{
		NvFlexDestroyDistanceField(g_flexLib, iter.first);
		DestroyGpuMesh(iter.second); // OpenGL
	}

	for (auto& iter : g_convexes)
	{
		NvFlexDestroyConvexMesh(g_flexLib, iter.first);
		DestroyGpuMesh(iter.second); // OpenGL
	}

	g_fields.clear();
	g_meshes.clear();
	g_convexes.clear();
}

// TODO: Rename "reset" (init has already happened by this time)
void Init(int scene, bool centerCamera)
{
	cout<<"Init"<<endl;
	RandInit();

	ResetSimGlobals(); // maps buffers
	ResetVisGlobals();

	ResetCoupledAssets();
	ResetSim(); // unmaps buffers

	if (!g_renderOff) {
		MapBuffers(g_buffers);
		ResetRender(centerCamera);
		UnmapBuffers(g_buffers);
	}

}

void Reset()
{
	cout<<"Reset"<<endl;
	Init(g_scene, false);
}

void Shutdown()
{
	cout<<"Shutdown"<<endl;
	// free buffers
	DestroyBuffers(g_buffers);

	for (auto& iter : g_meshes)
	{
		NvFlexDestroyTriangleMesh(g_flexLib, iter.first);

		if (iter.second)
			DestroyGpuMesh(iter.second);
	}

	for (auto& iter : g_fields)
	{
		NvFlexDestroyDistanceField(g_flexLib, iter.first);

		if (iter.second)
			DestroyGpuMesh(iter.second);
	}

	for (auto& iter : g_convexes)
	{
		NvFlexDestroyConvexMesh(g_flexLib, iter.first);

		if (iter.second)
			DestroyGpuMesh(iter.second);
	}

	g_fields.clear();
	g_meshes.clear();
	g_convexes.clear();

	NvFlexDestroySolver(g_solver);
	NvFlexShutdown(g_flexLib);

	for (auto& s : g_scenes) {
		s->Destroy();
	}
}

void UpdateWind()
{
	//cout<<"Updatewind"<<endl;
	g_windTime += g_dt;
	//
	const Vec3 kWindDir = Vec3(3.0f, 0.0f, 15.0f);  //[wb] wind by Xiaolvzi
	//const Vec3 kWindDir = Vec3(3.0f, 15.0f, 0.0f);  //[wb]
	const float kNoise = Perlin1D(g_windTime*g_windFrequency, 10, 0.25f); //wind by Xiaolvzi
	// Vec3 wind = g_windStrength*kWindDir*Vec3(kNoise, fabsf(kNoise), 0.0f);
	Vec3 wind = g_windStrength*kWindDir*Vec3(kNoise, 0.0f, fabsf(kNoise));

	g_params.wind[0] = wind.x;
	g_params.wind[1] = wind.y;
	g_params.wind[2] = wind.z;

	if (g_wavePool)
	{
		g_waveTime += g_dt;

		g_params.planes[2][3] = g_wavePlane + (sinf(float(g_waveTime)*g_waveFrequency - kPi*0.5f)*0.5f + 0.5f)*g_waveAmplitude;
	}
}

void SyncScene()
{
	//cout<<"SyncScene"<<endl;
	// let the scene send updates to flex directly
	g_scenes[g_scene]->Sync();
}

void UpdateScene()
{
	//cout<<"UpdateScene"<<endl;
	// give scene a chance to make changes to particle buffers
	g_scenes[g_scene]->Update();
}

void RenderScene()
{
	cout<<"RenderScene"<<endl;
	const int numParticles = NvFlexGetActiveCount(g_solver);
	const int numDiffuse = g_buffers->diffuseCount[0];

	//---------------------------------------------------
	// use VBO buffer wrappers to allow Flex to write directly to the OpenGL buffers
	// Flex will take care of any CUDA interop mapping/unmapping during the get() operations

	if (numParticles)
	{

		if (g_interop)
		{
			// copy data directly from solver to the renderer buffers
			UpdateFluidRenderBuffers(g_fluidRenderBuffers, g_solver, g_drawEllipsoids, g_drawDensity);
		}
		else
		{
			// copy particle data to GPU render device

			if (g_drawEllipsoids)
			{
				// if fluid surface rendering then update with smooth positions and anisotropy
				UpdateFluidRenderBuffers(g_fluidRenderBuffers,
					&g_buffers->smoothPositions[0],
					(g_drawDensity) ? &g_buffers->densities[0] : (float*)&g_buffers->phases[0],
					&g_buffers->anisotropy1[0],
					&g_buffers->anisotropy2[0],
					&g_buffers->anisotropy3[0],
					g_buffers->positions.size(),
					&g_buffers->activeIndices[0],
					numParticles);
			}
			else
			{
				// otherwise just send regular positions and no anisotropy
				UpdateFluidRenderBuffers(g_fluidRenderBuffers,
					&g_buffers->positions[0],
					(float*)&g_buffers->phases[0],
					NULL, NULL, NULL,
					g_buffers->positions.size(),
					&g_buffers->activeIndices[0],
					numParticles);
			}
		}
	}

	// GPU Render time doesn't include CPU->GPU copy time
	GraphicsTimerBegin();
	
	if (numDiffuse)
	{
		if (g_interop)
		{
			// copy data directly from solver to the renderer buffers
			UpdateDiffuseRenderBuffers(g_diffuseRenderBuffers, g_solver);
		}
		else
		{
			// copy diffuse particle data from host to GPU render device 
			UpdateDiffuseRenderBuffers(g_diffuseRenderBuffers,
				&g_buffers->diffusePositions[0],
				&g_buffers->diffuseVelocities[0],
				numDiffuse);
		}
	}
	
	//---------------------------------------
	// setup view and state

	float fov = kPi / 4.0f;
	float aspect = float(g_screenWidth) / g_screenHeight;

	Matrix44 proj = ProjectionMatrix(RadToDeg(fov), aspect, g_camNear, g_camFar);
	Matrix44 view = RotationMatrix(-g_camAngle.x, Vec3(0.0f, 1.0f, 0.0f))*RotationMatrix(-g_camAngle.y, Vec3(cosf(-g_camAngle.x), 0.0f, sinf(-g_camAngle.x)))*TranslationMatrix(-Point3(g_camPos));

	//------------------------------------
	// lighting pass

	// expand scene bounds to fit most scenes
	// g_sceneLower = Min(g_sceneLower, Vec3(-2.0f, 0.0f, -2.0f));
	// g_sceneUpper = Max(g_sceneUpper, Vec3(2.0f, 2.0f, 2.0f));

	Vec3 sceneExtents = g_sceneUpper - g_sceneLower;
	Vec3 sceneCenter = 0.5f*(g_sceneUpper + g_sceneLower);

	g_lightDir = Normalize(Vec3(5.0f, 15.0f, 7.5f));
	g_lightPos = sceneCenter + g_lightDir*Length(sceneExtents)*g_lightDistance;
	g_lightTarget = sceneCenter;

	// calculate tight bounds for shadow frustum
	float lightFov = g_lightFovScale*atanf(Length(g_sceneUpper - sceneCenter) / Length(g_lightPos - sceneCenter));
	// float lightFov = 2.0f*atanf(Length(g_sceneUpper - sceneCenter) / Length(g_lightPos - sceneCenter));

	// scale and clamp fov for aesthetics
	lightFov = Clamp(lightFov, DegToRad(25.0f), DegToRad(65.0f));

	Matrix44 lightPerspective = ProjectionMatrix(RadToDeg(lightFov), 1.0f, 1.0f, 1000.0f);
	Matrix44 lightView = LookAtMatrix(Point3(g_lightPos), Point3(g_lightTarget));
	Matrix44 lightTransform = lightPerspective*lightView;

	// radius used for drawing
	float radius = Max(g_params.solidRestDistance, g_params.fluidRestDistance)*0.5f*g_pointScale;

	//-------------------------------------
	// shadowing pass 

	if (g_meshSkinIndices.size())
		SkinMesh();

	// create shadow maps
	ShadowBegin(g_shadowMap);

	SetView(lightView, lightPerspective);
	SetCullMode(false);

	// give scene a chance to do custom drawing
	g_scenes[g_scene]->Draw(1);

	if (g_drawMesh)
		DrawMesh(g_mesh, g_meshColor);

	DrawShapes();

	if (g_drawCloth && g_buffers->triangles.size())
	{
		DrawCloth(
			&g_buffers->positions[0],
			&g_buffers->normals[0],
			g_buffers->uvs.size() ? &g_buffers->uvs[0].x : NULL,
			&g_buffers->triangles[0],
			g_buffers->triangles.size() / 3,
			g_buffers->positions.size(),
			g_clothColorIndex,
			g_expandCloth
		);
	}

	int shadowParticles = numParticles;
	int shadowParticlesOffset = 0;

	if (!g_drawPoints)
	{
		shadowParticles = 0;

		if (g_drawEllipsoids)
		{
			shadowParticles = numParticles - g_numSolidParticles;
			shadowParticlesOffset = g_numSolidParticles;
		}
	}
	else
	{
		int offset = g_drawMesh ? g_numSolidParticles : 0;

		shadowParticles = numParticles - offset;
		shadowParticlesOffset = offset;
	}

	if (g_buffers->activeIndices.size())
		DrawPoints(g_fluidRenderBuffers, shadowParticles, shadowParticlesOffset, radius, 2048, 1.0f, lightFov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_drawDensity);

	ShadowEnd();

	//----------------
	// lighting pass

	BindSolidShader(g_lightPos, g_lightTarget, lightTransform, g_shadowMap, 0.0f, Vec4(g_clearColor, g_fogDistance));

	SetView(view, proj);
	SetCullMode(true);

	// When the benchmark measures async compute, we need a graphics workload that runs for a whole frame.
	// We do this by rerendering our simple graphics many times.
	int passes = g_increaseGfxLoadForAsyncComputeTesting ? 50 : 1;

	for (int i = 0; i != passes; i++)
	{

		// Draw the floor
		DrawPlanes((Vec4*)g_params.planes, g_params.numPlanes, g_drawPlaneBias);

		if (g_drawMesh)
			DrawMesh(g_mesh, g_meshColor);


		DrawShapes();

		if (g_drawCloth && g_buffers->triangles.size())
			//DrawCloth(&g_buffers->positions[0], &g_buffers->normals[0], g_buffers->uvs.size() ? &g_buffers->uvs[0].x : NULL, &g_buffers->triangles[0], g_buffers->triangles.size() / 3, g_buffers->positions.size(), 3, g_expandCloth);
			DrawCloth(
				&g_buffers->positions[0],
				&g_buffers->normals[0],
				g_buffers->uvs.size() ? &g_buffers->uvs[0].x : NULL,
				&g_buffers->triangles[0],
				g_buffers->triangles.size() / 3,
				g_buffers->positions.size(),
				g_clothColorIndex,
				g_expandCloth
			);

		// give scene a chance to do custom drawing
		g_scenes[g_scene]->Draw(0);
	}
	UnbindSolidShader();


	// first pass of diffuse particles (behind fluid surface)
	if (g_drawDiffuse)
		RenderDiffuse(g_fluidRenderer, g_diffuseRenderBuffers, numDiffuse, radius*g_diffuseScale, float(g_screenWidth), aspect, fov, g_diffuseColor, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_diffuseMotionScale, g_diffuseInscatter, g_diffuseOutscatter, g_diffuseShadow, false);

	if (g_drawEllipsoids)
	{
		// draw solid particles separately
		if (g_numSolidParticles && g_drawPoints)
			DrawPoints(g_fluidRenderBuffers, g_numSolidParticles, 0, radius, float(g_screenWidth), aspect, fov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_drawDensity);

		// render fluid surface
		RenderEllipsoids(g_fluidRenderer, g_fluidRenderBuffers, numParticles - g_numSolidParticles, g_numSolidParticles, radius, float(g_screenWidth), aspect, fov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_fluidColor, g_blur, g_ior, g_drawOpaque);

		// second pass of diffuse particles for particles in front of fluid surface
		if (g_drawDiffuse)
			RenderDiffuse(g_fluidRenderer, g_diffuseRenderBuffers, numDiffuse, radius*g_diffuseScale, float(g_screenWidth), aspect, fov, g_diffuseColor, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_diffuseMotionScale, g_diffuseInscatter, g_diffuseOutscatter, g_diffuseShadow, true);
	}
	else
	{
		// draw all particles as spheres
		if (g_drawPoints)
		{
			int offset = g_drawMesh ? g_numSolidParticles : 0;

			if (g_buffers->activeIndices.size())
				DrawPoints(g_fluidRenderBuffers, numParticles - offset, offset, radius, float(g_screenWidth), aspect, fov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_drawDensity);
		}
	}

	GraphicsTimerEnd();
}

void RenderDebug()
{
	cout<<"RenderDebug"<<endl;
	if (g_mouseParticle != -1)
	{
		// draw mouse spring
		BeginLines();
		DrawLine(g_mousePos, Vec3(g_buffers->positions[g_mouseParticle]), Vec4(1.0f));
		EndLines();
	}

	// springs
	if (g_drawSprings)
	{
		Vec4 color;

		if (g_drawSprings == 1)
		{
			// stretch 
			color = Vec4(0.0f, 0.0f, 1.0f, 0.8f);
		}
		if (g_drawSprings == 2)
		{
			// tether
			color = Vec4(0.0f, 1.0f, 0.0f, 0.8f);
		}

		BeginLines();

		int start = 0;

		for (int i = start; i < g_buffers->springLengths.size(); ++i)
		{
			if (g_drawSprings == 1 && g_buffers->springStiffness[i] < 0.0f)
				continue;
			if (g_drawSprings == 2 && g_buffers->springStiffness[i] > 0.0f)
				continue;

			int a = g_buffers->springIndices[i * 2];
			int b = g_buffers->springIndices[i * 2 + 1];

			DrawLine(Vec3(g_buffers->positions[a]), Vec3(g_buffers->positions[b]), color);
		}

		EndLines();
	}

	// visualize contacts against the environment
	if (g_drawContacts)
	{
		const int maxContactsPerParticle = 6;

		NvFlexVector<Vec4> contactPlanes(g_flexLib, g_buffers->positions.size()*maxContactsPerParticle);
		NvFlexVector<Vec4> contactVelocities(g_flexLib, g_buffers->positions.size()*maxContactsPerParticle);
		NvFlexVector<int> contactIndices(g_flexLib, g_buffers->positions.size());
		NvFlexVector<unsigned int> contactCounts(g_flexLib, g_buffers->positions.size());

		NvFlexGetContacts(g_solver, contactPlanes.buffer, contactVelocities.buffer, contactIndices.buffer, contactCounts.buffer);

		// ensure transfers have finished
		contactPlanes.map();
		contactVelocities.map();
		contactIndices.map();
		contactCounts.map();

		BeginLines();

		for (int i = 0; i < int(g_buffers->activeIndices.size()); ++i)
		{
			const int contactIndex = contactIndices[g_buffers->activeIndices[i]];
			const unsigned int count = contactCounts[contactIndex];

			const float scale = 0.1f;

			for (unsigned int c = 0; c < count; ++c)
			{
				Vec4 plane = contactPlanes[contactIndex*maxContactsPerParticle + c];

				DrawLine(Vec3(g_buffers->positions[g_buffers->activeIndices[i]]), 
						 Vec3(g_buffers->positions[g_buffers->activeIndices[i]]) + Vec3(plane)*scale,
						 Vec4(0.0f, 1.0f, 0.0f, 0.0f));
			}
		}

		EndLines();
	}
	
	if (g_drawBases)
	{
		for (int i = 0; i < int(g_buffers->rigidRotations.size()); ++i)
		{
			BeginLines();

			float size = 0.1f;

			for (int b = 0; b < 3; ++b)
			{
				Vec3 color;
				color[b] = 1.0f;
			
				Matrix33 frame(g_buffers->rigidRotations[i]);

				DrawLine(Vec3(g_buffers->rigidTranslations[i]),
						 Vec3(g_buffers->rigidTranslations[i] + frame.cols[b] * size),
						 Vec4(color, 0.0f));
			}

			EndLines();
		}
	}

	if (g_drawNormals)
	{
		NvFlexGetNormals(g_solver, g_buffers->normals.buffer, NULL);

		BeginLines();

		for (int i = 0; i < g_buffers->normals.size(); ++i)
		{
			DrawLine(Vec3(g_buffers->positions[i]),
					 Vec3(g_buffers->positions[i] - g_buffers->normals[i] * g_buffers->normals[i].w),
					 Vec4(0.0f, 1.0f, 0.0f, 0.0f));
		}

		EndLines();
	}
}

void DrawShapes()
{
	cout<<"DrawShapes"<<endl;
	for (int i = 0; i < g_buffers->shapeFlags.size(); ++i)
	{
		const int flags = g_buffers->shapeFlags[i];

		// unpack flags
		int type = int(flags&eNvFlexShapeFlagTypeMask);
		//bool dynamic = int(flags&eNvFlexShapeFlagDynamic) > 0;

		Vec3 color = Vec3(0.9f);

		if (flags & eNvFlexShapeFlagTrigger)
		{
			color = Vec3(0.6f, 1.0, 0.6f);

			SetFillMode(true);		
		}

		// render with prev positions to match particle update order
		// can also think of this as current/next
		const Quat rotation = g_buffers->shapePrevRotations[i];
		const Vec3 position = Vec3(g_buffers->shapePrevPositions[i]);

		NvFlexCollisionGeometry geo = g_buffers->shapeGeometry[i];

		if (type == eNvFlexShapeSphere)
		{
			Mesh* sphere = CreateSphere(20, 20, geo.sphere.radius);

			Matrix44 xform = TranslationMatrix(Point3(position))*RotationMatrix(Quat(rotation));
			sphere->Transform(xform);

			DrawMesh(sphere, Vec3(color));

			delete sphere;
		}
		else if (type == eNvFlexShapeCapsule)
		{
			Mesh* capsule = CreateCapsule(10, 20, geo.capsule.radius, geo.capsule.halfHeight);

			// transform to world space
			Matrix44 xform = TranslationMatrix(Point3(position))*RotationMatrix(Quat(rotation))*RotationMatrix(DegToRad(-90.0f), Vec3(0.0f, 0.0f, 1.0f));
			capsule->Transform(xform);

			DrawMesh(capsule, Vec3(color));

			delete capsule;
		}
		else if (type == eNvFlexShapeBox)
		{
			Mesh* box = CreateCubeMesh();

			Matrix44 xform = TranslationMatrix(Point3(position))*RotationMatrix(Quat(rotation))*ScaleMatrix(Vec3(geo.box.halfExtents)*2.0f);
			box->Transform(xform);

			DrawMesh(box, Vec3(color));
			delete box;			
		}
		else if (type == eNvFlexShapeConvexMesh)
		{
			if (g_convexes.find(geo.convexMesh.mesh) != g_convexes.end())
			{
				GpuMesh* m = g_convexes[geo.convexMesh.mesh];

				if (m)
				{
					Matrix44 xform = TranslationMatrix(Point3(g_buffers->shapePositions[i]))*RotationMatrix(Quat(g_buffers->shapeRotations[i]))*ScaleMatrix(geo.convexMesh.scale);
					DrawGpuMesh(m, xform, Vec3(color));
				}
			}
		}
		else if (type == eNvFlexShapeTriangleMesh)
		{
			if (g_meshes.find(geo.triMesh.mesh) != g_meshes.end())
			{
				GpuMesh* m = g_meshes[geo.triMesh.mesh];

				if (m)
				{
					Matrix44 xform = TranslationMatrix(Point3(position))*RotationMatrix(Quat(rotation))*ScaleMatrix(geo.triMesh.scale);
					DrawGpuMesh(m, xform, Vec3(color));
				}
			}
		}
		else if (type == eNvFlexShapeSDF)
		{
			if (g_fields.find(geo.sdf.field) != g_fields.end())
			{
				GpuMesh* m = g_fields[geo.sdf.field];

				if (m)
				{
					Matrix44 xform = TranslationMatrix(Point3(position))*RotationMatrix(Quat(rotation))*ScaleMatrix(geo.sdf.scale);
					DrawGpuMesh(m, xform, Vec3(color));
				}
			}
		}
	}

	SetFillMode(g_wireframe);
}

void UpdateSimMapped() {
	//cout<<"UpdateSimMapped"<<endl;

	if (!g_pause || g_step)	
	{	
		UpdateWind();
		UpdateScene();
	}

	if (g_exportObjsFlag || g_saveClothPerSimStep) {
		printf("\n\nsaving cloth for frame %d\n", g_frame);
		g_scenes[g_scene]->Export(&g_exportBase[0]);
		// ExportObjs(&g_clothObjPath[0], &g_transformedMeshPath[0]);
	}
}

int UpdateRenderMapped() {
	cout<<"UpdateRenderMapped"<<endl;

	if (!g_fixCamera) {
		OrientCamera(g_camLateralAngle, g_camDownTilt);
	}

	if (!g_pause || g_step)	
	{	
		UpdateMouse();
	}

	StartFrame(Vec4(g_clearColor, 1.0f));

	// main scene render
	RenderScene();
	RenderDebug();
	
	int newScene = DoUI();

	EndFrame();

	// If user has disabled async compute, ensure that no compute can overlap 
	// graphics by placing a sync between them	
	if (!g_useAsyncCompute)
		NvFlexComputeWaitForGraphics(g_flexLib);

	return newScene;
}

void UpdateSimPostmap() {
	//cout<<"UpdateSimPostmap"<<endl;

	// auto& particles = g_buffers->positions;
	// auto c1 = 0;
	// auto c2 = particle.size()/2;

	// g_buffers->velocities[c1] = 0.0f; //Vec3(0.0, 0.0, 0.0);
	// g_buffers->velocities[c2] = 0.0f; //Vec3(0.0, 0.0, 0.0);
	//g_buffers->positions[c1].w = 0.0f;
	//g_buffers->positions[c2].w = 0.0f;
	//printf("111111");
	//printf(g_buffers->positions[0]);


	NvFlexSetParticles(g_solver, g_buffers->positions.buffer, NULL);
	NvFlexSetVelocities(g_solver, g_buffers->velocities.buffer, NULL);
	NvFlexSetPhases(g_solver, g_buffers->phases.buffer, NULL);
	NvFlexSetActive(g_solver, g_buffers->activeIndices.buffer, NULL);
	NvFlexSetActiveCount(g_solver, g_buffers->activeIndices.size());

	// allow scene to update constraints etc
	SyncScene();


	

	if (g_shapesChanged)
	{
		NvFlexSetShapes(
			g_solver,
			g_buffers->shapeGeometry.buffer,
			g_buffers->shapePositions.buffer,
			g_buffers->shapeRotations.buffer,
			g_buffers->shapePrevPositions.buffer,
			g_buffers->shapePrevRotations.buffer,
			g_buffers->shapeFlags.buffer,
			int(g_buffers->shapeFlags.size()));

		g_shapesChanged = false;
	}

	if (!g_pause || g_step)
	{
		// tick solver
		NvFlexSetParams(g_solver, &g_params);
		NvFlexUpdateSolver(g_solver, g_dt, g_numSubsteps, g_profile);

		g_frame++;
		g_step = false;
	}

	// read back base particle data
	// Note that FlexGet is asynchronously queued; guaranteed to finish
	//	only when synced (e.g. when mapping buffers)
	NvFlexGetParticles(g_solver, g_buffers->positions.buffer, NULL);
	NvFlexGetVelocities(g_solver, g_buffers->velocities.buffer, NULL);
	NvFlexGetNormals(g_solver, g_buffers->normals.buffer, NULL);

	// readback triangle normals
	if (g_buffers->triangles.size())
		NvFlexGetDynamicTriangles(g_solver, g_buffers->triangles.buffer, g_buffers->triangleNormals.buffer, g_buffers->triangles.size() / 3);

	// readback rigid transforms
	if (g_buffers->rigidOffsets.size())
		NvFlexGetRigids(g_solver, NULL, NULL, NULL, NULL, NULL, NULL, NULL, g_buffers->rigidRotations.buffer, g_buffers->rigidTranslations.buffer);

}

void UpdateRenderPostmap() {
	cout<<"UpdateRenderPostmap"<<endl;

	// move mouse particle (must be done here as GetViewRay() uses the GL projection state)
	if (g_mouseParticle != -1)
	{
		Vec3 origin, dir;
		GetViewRay(g_lastx, g_screenHeight - g_lasty, origin, dir);

		g_mousePos = origin + dir*g_mouseT;
	}

	if (g_capture)
	{
		SaveFrame(g_snapFile);
	}

}

// TODO: Try to decouple as much as possible or find a way to refactor out the various "if (!g_renderOff)" statements
void UpdateFrame(bool render=true)
{
	//cout<<"Update Frame"<<endl;
	static double lastTime;

	// real elapsed frame time
	double frameBeginTime = GetSeconds();

	g_realdt = float(frameBeginTime - lastTime);
	lastTime = frameBeginTime;

	//-------------------------------------------------------------------
	// Scene Update

	double waitBeginTime = GetSeconds();

	MapBuffers(g_buffers);

	double waitEndTime = GetSeconds();

	// Getting timers causes CPU/GPU sync, so we do it after a map
	float newSimLatency = NvFlexGetDeviceLatency(g_solver, &g_GpuTimers.computeBegin, &g_GpuTimers.computeEnd, &g_GpuTimers.computeFreq);
	// below call simply returns 0

	UpdateSimMapped();

	//-------------------------------------------------------------------
	// Render

	double renderBeginTime = GetSeconds();

	if (g_profile && (!g_pause || g_step))
	{
		if (g_benchmark)
		{
			g_numDetailTimers = NvFlexGetDetailTimers(g_solver, &g_detailTimers);
		}
		else
		{
			memset(&g_timers, 0, sizeof(g_timers));
			NvFlexGetTimers(g_solver, &g_timers);
		}
	}

	int newScene = -1;

	if (render) {
		newScene = UpdateRenderMapped();
	}

	UnmapBuffers(g_buffers);

	if (render)
		UpdateRenderPostmap();

	double renderEndTime = GetSeconds();

	// if user requested a scene reset process it now
	if (g_resetScene) 
	{
		Reset();
		g_resetScene = false;
	}

	//-------------------------------------------------------------------
	// Flex Update

	double updateBeginTime = GetSeconds();

	UpdateSimPostmap();

	if (!g_interop)
	{
		// if not using interop then we read back fluid data to host
		if (g_drawEllipsoids)
		{
			NvFlexGetSmoothParticles(g_solver, g_buffers->smoothPositions.buffer, NULL);
			NvFlexGetAnisotropy(g_solver, g_buffers->anisotropy1.buffer, g_buffers->anisotropy2.buffer, g_buffers->anisotropy3.buffer, NULL);
		}

		// read back diffuse data to host
		if (g_drawDensity)
			NvFlexGetDensities(g_solver, g_buffers->densities.buffer, NULL);

		if (GetNumDiffuseRenderParticles(g_diffuseRenderBuffers))
		{
			NvFlexGetDiffuseParticles(g_solver, g_buffers->diffusePositions.buffer, g_buffers->diffuseVelocities.buffer, g_buffers->diffuseCount.buffer);
		}
	}
	else
	{
		// read back just the new diffuse particle count, render buffers will be updated during rendering
		NvFlexGetDiffuseParticles(g_solver, NULL, NULL, g_buffers->diffuseCount.buffer);	
	}


	double updateEndTime = GetSeconds();

	//-------------------------------------------------------
	// Update the on-screen timers

	float newUpdateTime = float(updateEndTime - updateBeginTime);
	float newRenderTime = float(renderEndTime - renderBeginTime);
	float newWaitTime = float(waitBeginTime - waitEndTime);

	// Exponential filter to make the display easier to read
	const float timerSmoothing = 0.05f;

	g_updateTime = (g_updateTime == 0.0f) ? newUpdateTime : Lerp(g_updateTime, newUpdateTime, timerSmoothing);
	g_renderTime = (g_renderTime == 0.0f) ? newRenderTime : Lerp(g_renderTime, newRenderTime, timerSmoothing);
	g_waitTime = (g_waitTime == 0.0f) ? newWaitTime : Lerp(g_waitTime, newWaitTime, timerSmoothing);
	g_simLatency = (g_simLatency == 0.0f) ? newSimLatency : Lerp(g_simLatency, newSimLatency, timerSmoothing);
	
	if(g_benchmark) newScene = BenchmarkUpdate();

	// if gui or benchmark requested a scene change process it now
	if (newScene != -1)
	{
		g_scene = newScene;
		Init(g_scene);
	}

	if (render)
		PresentFrame(g_vsync);
}

void PhotoShoot() {
	cout<<"PhotoShoot"<<endl;

	float tilts[3] = { 0.0f, 15.0f, 35.0f };

	constexpr int n_angles = 4;
	float angles[n_angles];

	for (int i = 0; i < n_angles; ++i) {
		angles[i] = (360.0f / n_angles) * i;
	}

	string base(g_snapFile);
	base = base.substr(0, base.find_last_of("."));

	g_capture = true;
	g_pause = true;

	for (int i = 0; i < n_angles; ++i) {

		for (int j = 0; j < 3; ++j) {

			strcpy(g_snapFile, (base + "_a" + to_string(i) + "_tilt" + to_string(j) + ".png").c_str());

			g_camLateralAngle = angles[i];
			g_camDownTilt = tilts[j];

			UpdateFrame();
		}
	}

	g_capture = false;
	g_pause = false;
}


void MainLoop(int n_iters)
{
	cout<<"MainLoop"<<endl;
	// printf("\n\nhere\n\n");
	// printf ("\nvalllll %d \n", g_renderOff);
	// printf ("\n\ndoen\n\n");
	// exit(-1);
	bool quit = false;
	SDL_Event e;

	while (g_frame < n_iters && !quit && !g_quit)
	{

		if (g_frame % 20 == 0) {
			printf("Frame %d\n", g_frame);
		}

		bool isLast = g_frame == n_iters-1;

		if (isLast) {

			if (g_exportObjs) {
				g_exportObjsFlag = true;
			}

			if (g_takeSnaps) {
				if (g_renderOff) {
					InitRender();
					ResetRender(true);
					g_renderOff = false;
				}

				if (!g_multishot) {
					g_capture = true;
				}
			}
		}

		UpdateFrame(!g_renderOff);
		quit = ProcessInput(e);

		if (isLast) {
			if (g_takeSnaps && g_multishot) {
				g_exportObjsFlag = false;
				PhotoShoot();
			}

            // do final things
            /*
			Amir's edit: disable doing unnecessary calculations
			if (strcmp(g_scenes[g_scene]->mName, "Occlusion") == 0) {
				MapBuffers(g_buffers);
				Occlusion *scene = static_cast<Occlusion*>(g_scenes[g_scene]);
				
				float max_d = scene->ComputeMaxNeighborDist();
				printf("Max inter-particle distance at rest: %f\n", max_d);
				UnmapBuffers(g_buffers);
			}
			*/
		}
	}
}

void setCapture() {
	g_showHelp = false;
	g_takeSnaps = true;
}

bool exists(const char* p, bool verbose=false) {
	FILE *f = fopen(p, "r");
	if (f) {
		fclose(f);
		return true;
	} else {
		if (verbose) {
			fprintf(stderr, "Error: %s does not exist.\n", p);
		}
		return false;
	}
}


int main(int argc, char* argv[])
{
	// process command line args
	srand ( time(NULL) );
	char objpath[400];
	bool objpath_exists = false;
	float particle_radius = 0.0f;
	float stretch_stiffness = 1.0f;
	float bend_stiffness = 0.8f;
	float shear_stiffness = 0.5f;
    float extra_cp_spacing = 0.0f;
    float extra_cp_rad_mult = 1.0f;
	int cloth_size = Wind::AUTO_CLOTH_SIZE; // will autocompute
	int iters = 5000;
	float obj_scale = 2.0;
	Vec3 translate = Vec3(0,0,0);
	Vec4 rotate = Vec4(0,0,0,0);
    bool use_quat = false;
    char cfg[200];
    bool parse = false;


	for (int i = 1; i < argc; ++i)
	{
		int d;
		float f;

		if (sscanf(argv[i], "-device=%d", &d))
			g_device = d;

		if (sscanf(argv[i], "-extensions=%d", &d))
			g_extensions = d != 0;

		if (string(argv[i]).find("-benchmark") != string::npos)
		{
			g_benchmark = true;
			g_profile = true;
			g_outputAllFrameTimes = false;
			g_vsync = false;
			g_fullscreen = true;
		}

		if (string(argv[i]).find("-benchmarkAllFrameTimes") != string::npos)
		{
			g_benchmark = true;
			g_outputAllFrameTimes = true;
		}

		if (string(argv[i]).find("-tc") != string::npos)
		{
			g_teamCity = true;
		}

		if (sscanf(argv[i], "-msaa=%d", &d))
			g_msaaSamples = d;

		int w = 1280;
		int h = 720;
		if (sscanf(argv[i], "-fullscreen=%dx%d", &w, &h) == 2)
		{
			g_screenWidth = w;
			g_screenHeight = h;
			g_fullscreen = true;
		}
		else if (string(argv[i]).find("-fullscreen") != string::npos)
		{
			g_screenWidth = w;
			g_screenHeight = h;
			g_fullscreen = true;
		}

		if (sscanf(argv[i], "-windowed=%dx%d", &w, &h) == 2)
		{
			g_screenWidth = w;
			g_screenHeight = h;
			g_fullscreen = false;
		}
		else if (strstr(argv[i], "-windowed"))
		{
			g_screenWidth = w;
			g_screenHeight = h;
			g_fullscreen = false;
		}

		if (sscanf(argv[i], "-vsync=%d", &d))
			g_vsync = d != 0;

		if (sscanf(argv[i], "-multiplier=%d", &d) == 1)
		{
			g_numExtraMultiplier = d;
		}

		if (string(argv[i]).find("-disabletweak") != string::npos)
		{
			g_tweakPanel = false;
		}

		if (string(argv[i]).find("-disableinterop") != string::npos)
		{
			g_interop = false;
		}
		if (sscanf(argv[i], "-asynccompute=%d", &d) == 1)
		{
			g_useAsyncCompute = (d != 0);
		}

		// for occlusion sims
		if (sscanf(argv[i], "-obj=%s", objpath) == 1) {
			if (!exists(&objpath[0])) {
				exit(-1);
			}
			objpath_exists = true;
		}
        if (sscanf(argv[i], "-extraspace=%f", &f) == 1)
		{
			extra_cp_spacing = f;
		}
        if (sscanf(argv[i], "-extramult=%f", &f) == 1)
		{
			extra_cp_rad_mult = f;
		}
		if (sscanf(argv[i], "-bstiff=%f", &f) == 1)
		{
			bend_stiffness = f;
		}
		if (sscanf(argv[i], "-shstiff=%f", &f) == 1)
		{
			shear_stiffness = f;
		}
		if (sscanf(argv[i], "-ststiff=%f", &f) == 1)
		{
			stretch_stiffness = f;
		}
		if (sscanf(argv[i], "-stiff_scale=%f", &f) == 0.8)
		{
			bend_stiffness *= f;
			shear_stiffness *= f;
			stretch_stiffness *= f;
		}
		if (sscanf(argv[i], "-iters=%d", &d) == 1)
		{
			iters = d;
		}

		if (sscanf(argv[i], "-scale=%f", &f) == 1)
		{
			obj_scale = f;
		}
		if (sscanf(argv[i], "-x=%f", &f) == 1)
		{
			translate.x = f;
		}
		if (sscanf(argv[i], "-y=%f", &f) == 1)
		{
			translate.y = f;
		}
		if (sscanf(argv[i], "-z=%f", &f) == 1)
		{
			translate.z = f;
		}
		if (sscanf(argv[i], "-rx=%f", &f) == 1)
		{
			rotate.x = f;
		}
		if (sscanf(argv[i], "-ry=%f", &f) == 1)
		{
			rotate.y = f;
		}
		if (sscanf(argv[i], "-rz=%f", &f) == 1)
		{
			rotate.z = f;
		}
		if (sscanf(argv[i], "-rw=%f", &f) == 1)
		{
			rotate.w = f;
		}
		if (string(argv[i]).find("-use_quat") != string::npos)
		{
			use_quat=true;
		}
        if (string(argv[i]).find("-use_euler") != string::npos)
		{
			use_quat=false;
		}
		if (sscanf(argv[i], "-ss=%d", &d) == 1)
		{
			g_numSubsteps = d;
		}
		if (sscanf(argv[i], "-si=%d", &d) == 1)
		{
			g_params.numIterations = d;
		}
		if (sscanf(argv[i], "-dt=%f", &f) == 1)
		{
			g_dt = f;
		}

		// for decoupled rendering
		if (string(argv[i]).find("-offline") != string::npos)
		{
			g_renderOff = true;
		}
		if (string(argv[i]).find("-img") != string::npos)
		{
			setCapture();
		}
		if (sscanf(argv[i], "-img=%s", g_snapFile) == 1)
		{
			setCapture();
		}
		if (string(argv[i]).find("-photoshoot") != string::npos)
		{
			setCapture();
			g_multishot = true;
		}
		if (string(argv[i]).find("-export") != string::npos)
		{
			g_exportObjs = true;
		}
		if (sscanf(argv[i], "-export=%s", g_exportBase) == 1)
		{
			g_exportObjs = true;
		}
		if (string(argv[i]).find("-saveClothPerSimStep") != string::npos)
		{
			g_saveClothPerSimStep = true;
		}
		if ((string(argv[i]).find("-randomSeed") != string::npos) & (sscanf(argv[i], "-randomSeed=%d", &d) == 1))
		{
			g_randomSeed = d;
		}
		if ((string(argv[i]).find("-randomClothMinRes") != string::npos) & (sscanf(argv[i], "-randomClothMinRes=%d", &d) == 1))
		{
			g_randomClothMinRes = d;
		}
		if ((string(argv[i]).find("-randomClothMaxRes") != string::npos) & (sscanf(argv[i], "-randomClothMaxRes=%d", &d) == 1))
		{
			g_randomClothMaxRes = d;
		}

		// Cloth particles properties
		if ((string(argv[i]).find("-psize") != string::npos) & (sscanf(argv[i], "-psize=%f", &f) == 1))
		{
			particle_radius = f;
		}
		if ((string(argv[i]).find("-clothLift") != string::npos) & (sscanf(argv[i], "-clothLift=%f", &f) == 1))
		{
			g_clothLift = f;
		}
		if ((string(argv[i]).find("-clothStiffness") != string::npos) & (sscanf(argv[i], "-clothStiffness=%f", &f) == 1))
		{
			g_clothStiffness = f;
		}
		if ((string(argv[i]).find("-clothDrag") != string::npos) & (sscanf(argv[i], "-clothDrag=%f", &f) == 1))
		{
			g_clothDrag = f;
		}
		if ((string(argv[i]).find("-clothFriction") != string::npos) & (sscanf(argv[i], "-clothFriction=%f", &f) == 1))
		{
			g_clothFriction = f;
		}

		sscanf(argv[i], "-clothsize=%d", &d);
		// printf ("\n argument name %s and value is %d and comparison is %d\n", argv[i], d, (string(argv[i]).find("-clothsize") != string::npos));
		if ((string(argv[i]).find("-clothsize") != string::npos) & (d == 0) & (cloth_size == -1))
		{
			cloth_size = 0;
		}
		else if ((string(argv[i]).find("-clothsize") != string::npos) & (d > 0) & (cloth_size == -1))
		{
			g_clothNumParticles = d;
			cloth_size = d;
		}

		// aesthetics
		if (sscanf(argv[i], "-light=%f", &f) == 1)
		{
			g_lightFovScale = f;
		}
		if (sscanf(argv[i], "-ccolor_id=%d", &d) == 1)
		{
			g_clothColorIndex = d;
		}
		if (string(argv[i]).find("-v") != string::npos)
		{
			g_verbose = true;
		}
		if (string(argv[i]).find("-nofloor") != string::npos)
		{
			g_hasGround = false;
		}
		if (sscanf(argv[i], "-cam_dist=%f", &f) == 1)
		{
			g_cam_r = f;
		}
		if (string(argv[i]).find("-nocontacts") != string::npos)
		{
			g_exportContacts = false;
		}

		// load sim parameters from file
		if (sscanf(argv[i], "-config=%s", cfg) == 1) {
			if (!exists(&cfg[0], true)) {
				exit(-1);
			}
            parse = true;
		}
	}

	if (!objpath_exists) {
		sprintf(objpath, "../../data/armadillo.ply");
	}

	/*
	Amir's edit: We know the Quaternion is valid so we don't need this
    if (use_quat) {
        if (Length(rotate) > 1.0f) {
            printf("Invalid quaternion (%f, %f, %f) -- must have at most unit norm.\n", rotate.x, rotate.y, rotate.z);
            exit(-1);
        }
    }
    */
    /*
    Amir's edit: We assume the numbers come in radians already
    else {
       rotate.x = DegToRad(rotate.x);
       rotate.y = DegToRad(rotate.y);
       rotate.z = DegToRad(rotate.z);
    }
    */

    
	Wind::ClothParams cp(cloth_size, particle_radius, stretch_stiffness, bend_stiffness, shear_stiffness, extra_cp_spacing, extra_cp_rad_mult);
	Wind::ObjParams op(obj_scale, translate, rotate, use_quat);

    if (parse) {
        ParseConfig(&cfg[0], iters, cp, op);
    }

	g_scenes.push_back(new Wind("Wind", &objpath[0], cp, op));

	InitSim();

	if (!g_renderOff) {
		InitRender();
	}

	if (g_benchmark)
		g_scene = BenchmarkInit();

	// init default scene
	StartGpuWork();
	Init(g_scene);
	EndGpuWork();

	MainLoop(iters);

	if (g_fluidRenderer)
		DestroyFluidRenderer(g_fluidRenderer);

	if (g_fluidRenderBuffers)
		DestroyFluidRenderBuffers(g_fluidRenderBuffers);

	if (g_diffuseRenderBuffers)
		DestroyDiffuseRenderBuffers(g_diffuseRenderBuffers);

	if (g_shadowMap)
		ShadowDestroy(g_shadowMap);
	
	Shutdown();

	if (!g_renderOff) {
		DestroyRender();
		SDL_DestroyWindow(g_window);
		SDL_Quit();
	}

	return 0;
}
© 2020 GitHub, Inc.
Terms
Privacy
Security
Status
Help
Contact GitHub
Pricing
API
Training
Blog
About
