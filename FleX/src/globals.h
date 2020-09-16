#pragma once

// --------- GUI --------- //

#include "../external/SDL2-2.0.4/include/SDL.h"

SDL_Window* g_window;			// window handle
unsigned int g_windowId;		// window id

int g_screenWidth = 1280;
int g_screenHeight = 720;
int g_msaaSamples = 8;

/* Note that this array of colors is altered by demo code, and is also read from global by graphics API impls */
Colour g_colors[] =
{
	Colour(0.0f, 0.5f, 1.0f),				// cyan
	Colour(0.797f, 0.354f, 0.000f),	// gold
	Colour(0.092f, 0.465f, 0.820f), // cyan
	Colour(0.000f, 0.349f, 0.173f), // moss green
	Colour(0.875f, 0.782f, 0.051f), // yellow
	Colour(0.000f, 0.170f, 0.453f), // navy blue
	Colour(0.673f, 0.111f, 0.000f), // orange
	Colour(0.612f, 0.194f, 0.394f), // orchid

	// custom

};

bool g_exportContacts = true;

Vec3 g_camPos(6.0f, 8.0f, 18.0f);
Vec3 g_camAngle(0.0f, -DegToRad(20.0f), 0.0f);
Vec3 g_camVel(0.0f);
Vec3 g_camSmoothVel(0.0f);

// for main.cpp:OrientCamera
float g_camLateralAngle = 0.0f; // side-to-side angle (in degrees)
float g_camDownTilt = 0.0f;		// downward tilt (in degrees)
float g_cam_r = 6.0f; // fixed distance between camera and origin on xz-plane
Vec3 g_camOffset(0.0f, 0.0f, 0.0f);
bool g_fixCamera = false;

float g_camSpeed;
float g_camNear;
float g_camFar;

Vec3 g_lightPos;
Vec3 g_lightDir;
Vec3 g_lightTarget;

int g_scene = 0;
int g_selectedScene = g_scene;
int g_levelScroll;			// offset for level selection scroll area
bool g_resetScene = false;  //if the user clicks the reset button or presses the reset key this is set to true;

// mouse
int g_mouseParticle = -1;
float g_mouseT = 0.0f;
Vec3 g_mousePos;
float g_mouseMass;
bool g_mousePicked = false;
int g_lastx;
int g_lasty;
int g_lastb = -1;

ShadowMap* g_shadowMap = nullptr;

Vec4 g_fluidColor;
Vec4 g_diffuseColor;
Vec3 g_meshColor;
Vec3  g_clearColor;
float g_lightDistance;
float g_fogDistance;
float g_lightFovScale = 2.0f;

FluidRenderer* g_fluidRenderer;
FluidRenderBuffers* g_fluidRenderBuffers = nullptr;
DiffuseRenderBuffers* g_diffuseRenderBuffers = nullptr;

Vec3 g_sceneLower;
Vec3 g_sceneUpper;
Vec3 g_shapeLower;
Vec3 g_shapeUpper;

float g_blur;
float g_ior;
bool g_drawEllipsoids;
bool g_drawPoints;
bool g_drawMesh;
bool g_drawCloth;
float g_expandCloth;	// amount to expand cloth along normal (to account for particle radius)

bool g_drawOpaque;
int g_drawSprings;		// 0: no draw, 1: draw stretch 2: draw tether
bool g_drawBases = false;
bool g_drawContacts = false;
bool g_drawNormals = false;
bool g_drawDiffuse;
bool g_drawShapeGrid = false;
bool g_drawDensity = false;
float g_pointScale;
float g_drawPlaneBias;	// move planes along their normal for rendering
int g_clothColorIndex = 7;

bool g_capture = false;
bool g_showHelp = true;
bool g_tweakPanel = true;
bool g_fullscreen = false;
bool g_wireframe = false;

// mapping of collision mesh to render mesh
std::map<NvFlexConvexMeshId, GpuMesh*> g_convexes;
std::map<NvFlexTriangleMeshId, GpuMesh*> g_meshes;
std::map<NvFlexDistanceFieldId, GpuMesh*> g_fields;

// mesh used for deformable object rendering
Mesh* g_mesh;
vector<int> g_meshSkinIndices;
vector<float> g_meshSkinWeights;
vector<Point3> g_meshRestPositions;
const int g_numSkinWeights = 4;

// -------- FleX -------- //

#include "physics.h"

int g_numSubsteps = 5;
SimBuffers* g_buffers;

NvFlexSolver* g_solver;
NvFlexSolverDesc g_solverDesc;
NvFlexLibrary* g_flexLib;
NvFlexParams g_params = {0};

// a setting of -1 means Flex will use the device specified in the NVIDIA control panel
int g_device = -1;
char g_deviceName[256];

// ------- Profiling ------- //

float g_waitTime;		// the CPU time spent waiting for the GPU
float g_updateTime;     // the CPU time spent on Flex
float g_renderTime;		// the CPU time spent calling OpenGL to render the scene
                        // the above times don't include waiting for vsync
float g_simLatency;     // the time the GPU spent between the first and last NvFlexUpdateSolver() operation. Because some GPUs context switch, this can include graphics time.

bool g_profile = false;
bool g_outputAllFrameTimes = false;
bool g_asyncComputeBenchmark = false;

NvFlexTimers g_timers;
int g_numDetailTimers;
NvFlexDetailTimer * g_detailTimers;

bool g_hasGround = true;

// ------- Sim State ------- //

bool g_pause = false;
bool g_step = false;
bool g_debug = false;
bool g_verbose = false;
bool g_quit = false;
bool g_Error = false;

class Scene;
vector<Scene*> g_scenes;

// ----- Uncategorized ----- // TODO

// Cloth particles properties
int g_clothNumParticles = 210;
float g_particleRadius = 0.0078;
float g_clothLift = -1.0;
float g_clothStiffness = -1.0;
float g_clothDrag = -1.0;
float g_clothFriction = -1.0;

// Random cloth properties (note that the particle size will be chosen automatically depending on the randomly-sampled cloth particle numbers)
int g_randomSeed = -1;
int g_randomClothMinRes = 145;
int g_randomClothMaxRes = 215;


float g_windTime = 0.0f;
float g_windFrequency = 0.1f;
float g_windStrength = 0.0f;

bool g_wavePool = false;
float g_waveTime = 0.0f;
float g_wavePlane;
float g_waveFrequency = 1.5f;
float g_waveAmplitude = 1.0f;
float g_waveFloorTilt = 0.0f;

float g_diffuseScale;
float g_diffuseMotionScale;
bool g_diffuseShadow;
float g_diffuseInscatter;
float g_diffuseOutscatter;

float g_dt = 1.0f / 60.0f;	// the time delta used for simulation
float g_realdt;				// the real world time delta between updates

bool g_vsync = true;
bool g_benchmark = false;
bool g_extensions = true; // Enable or disable NVIDIA/AMD extensions in DirectX
bool g_teamCity = false;
bool g_interop = true;
bool g_useAsyncCompute = true;		
bool g_increaseGfxLoadForAsyncComputeTesting = false;

int g_maxDiffuseParticles;
unsigned char g_maxNeighborsPerParticle;
int g_numExtraParticles;
int g_numExtraMultiplier = 1;

// flag to request collision shapes be updated
bool g_shapesChanged = false;

// for offline rendering
bool g_renderOff = false;
bool g_takeSnaps = false;
bool g_multishot = false;
char g_snapFile[200] = "out.png";
bool g_exportObjs = false;
bool g_saveClothPerSimStep = false;
bool g_exportObjsFlag = false;
char g_exportBase[200] = "out";


bool g_emit = false;
bool g_warmup = false;

int g_frame = 0;
int g_numSolidParticles = 0;

FILE* g_ffmpeg;

// ------ Functions ------ //

void Init(int scene, bool centerCamera = true);


