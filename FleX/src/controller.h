#pragma once

#include "shaders.h"
#include "globals.h"

void ReshapeWindow(int width, int height)
{
	if (!g_benchmark) {
		printf("Reshaping\n\n");
	}

	ReshapeRender(g_window);

	if (!g_fluidRenderer || (width != g_screenWidth || height != g_screenHeight))
	{
		if (g_fluidRenderer)
			DestroyFluidRenderer(g_fluidRenderer);
		g_fluidRenderer = CreateFluidRenderer(width, height);
	}

	g_screenWidth = width;
	g_screenHeight = height;
}

void UpdateCamera()
{
	Vec3 forward(-sinf(g_camAngle.x)*cosf(g_camAngle.y), sinf(g_camAngle.y), -cosf(g_camAngle.x)*cosf(g_camAngle.y));
	Vec3 right(Normalize(Cross(forward, Vec3(0.0f, 1.0f, 0.0f))));

	g_camSmoothVel = Lerp(g_camSmoothVel, g_camVel, 0.1f);
	g_camPos += (forward*g_camSmoothVel.z + right*g_camSmoothVel.x + Cross(right, forward)*g_camSmoothVel.y);
}

void UpdateMouse()
{
	// mouse button is up release particle
	if (g_lastb == -1)
	{
		if (g_mouseParticle != -1)
		{
			// restore particle mass
			g_buffers->positions[g_mouseParticle].w = g_mouseMass;

			// deselect
			g_mouseParticle = -1;
		}
	}

	// mouse went down, pick new particle
	if (g_mousePicked)
	{
		assert(g_mouseParticle == -1);

		Vec3 origin, dir;
		GetViewRay(g_lastx, g_screenHeight - g_lasty, origin, dir);

		const int numActive = NvFlexGetActiveCount(g_solver);

		g_mouseParticle = PickParticle(origin, dir, &g_buffers->positions[0], &g_buffers->phases[0], numActive, g_params.radius*0.8f, g_mouseT);

		if (g_mouseParticle != -1)
		{
			printf("picked: %d, mass: %f pos: %f %f %f\n", g_mouseParticle, g_buffers->positions[g_mouseParticle].w, g_buffers->positions[g_mouseParticle].x, g_buffers->positions[g_mouseParticle].y, g_buffers->positions[g_mouseParticle].z);

			g_mousePos = origin + dir*g_mouseT;
			g_mouseMass = g_buffers->positions[g_mouseParticle].w;
			g_buffers->positions[g_mouseParticle].w = 0.0f;		// increase picked particle's mass to force it towards the point
		}

		g_mousePicked = false;
	}

	// update picked particle position
	if (g_mouseParticle != -1)
	{
		Vec3 p = Lerp(Vec3(g_buffers->positions[g_mouseParticle]), g_mousePos, 0.8f);
		Vec3 delta = p - Vec3(g_buffers->positions[g_mouseParticle]);

		g_buffers->positions[g_mouseParticle].x = p.x;
		g_buffers->positions[g_mouseParticle].y = p.y;
		g_buffers->positions[g_mouseParticle].z = p.z;

		g_buffers->velocities[g_mouseParticle].x = delta.x / g_dt;
		g_buffers->velocities[g_mouseParticle].y = delta.y / g_dt;
		g_buffers->velocities[g_mouseParticle].z = delta.z / g_dt;
	}
}

// ------- callbacks ------- //

void InputArrowKeysDown(int key, int x, int y)
{
	switch (key)
	{
	case SDLK_DOWN:
	{
		if (g_selectedScene < int(g_scenes.size()) - 1)
			g_selectedScene++;

		// update scroll UI to center on selected scene
		g_levelScroll = max((g_selectedScene - 4) * 24, 0);
		break;
	}
	case SDLK_UP:
	{
		if (g_selectedScene > 0)
			g_selectedScene--;

		// update scroll UI to center on selected scene
		g_levelScroll = max((g_selectedScene - 4) * 24, 0);
		break;
	}
	case SDLK_LEFT:
	{
		if (g_scene > 0)
			--g_scene;
		Init(g_scene);

		// update scroll UI to center on selected scene
		g_levelScroll = max((g_scene - 4) * 24, 0);
		break;
	}
	case SDLK_RIGHT:
	{
		if (g_scene < int(g_scenes.size()) - 1)
			++g_scene;
		Init(g_scene);

		// update scroll UI to center on selected scene
		g_levelScroll = max((g_scene - 4) * 24, 0);
		break;
	}
	}
}

void InputArrowKeysUp(int key, int x, int y)
{
}

bool InputKeyboardDown(unsigned char key, int x, int y)
{
	if (key > '0' && key <= '9')
	{
		g_scene = key - '0' - 1;
		Init(g_scene);
		return false;
	}

	float kSpeed = g_camSpeed;

	switch (key)
	{
	case 'w':
	{
		g_camVel.z = kSpeed;
		break;
	}
	case 's':
	{
		g_camVel.z = -kSpeed;
		break;
	}
	case 'a':
	{
		g_camVel.x = -kSpeed;
		break;
	}
	case 'd':
	{
		g_camVel.x = kSpeed;
		break;
	}
	case 'q':
	{
		g_camVel.y = kSpeed;
		break;
	}
	case 'z':
	{
		//g_drawCloth = !g_drawCloth;
		g_camVel.y = -kSpeed;
		break;
	}

	case 'u':
	{
		if (g_fullscreen)
		{
			SDL_SetWindowFullscreen(g_window, 0);
			ReshapeWindow(1280, 720);
			g_fullscreen = false;
		}
		else
		{
			SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			g_fullscreen = true;
		}
		break;
	}
	case 'r':
	{
		g_resetScene = true;
		break;
	}
	case 'y':
	{
		g_wavePool = !g_wavePool;
		break;
	}
	case 'c':
	{
		break;
	}
	case 'p':
	{
		g_pause = !g_pause;
		break;
	}
	case 'o':
	{
		g_step = true;
		break;
	}
	case 'h':
	{
		g_showHelp = !g_showHelp;
		break;
	}
	case 'e':
	{
		g_drawEllipsoids = !g_drawEllipsoids;
		break;
	}
	case 't':
	{
		g_drawOpaque = !g_drawOpaque;
		break;
	}
	case 'v':
	{
		g_drawPoints = !g_drawPoints;
		break;
	}
	case 'f':
	{
		g_drawSprings = (g_drawSprings + 1) % 3;
		break;
	}
	case 'i':
	{
		g_drawDiffuse = !g_drawDiffuse;
		break;
	}
	case 'm':
	{
		g_drawMesh = !g_drawMesh;
		break;
	}
	case 'j':
	{
		g_windTime = 0.0f;
		g_windStrength = 1.5f;
		g_windFrequency = 0.2f;
		break;
	}
	case '.':
	{
		g_profile = !g_profile;
		break;
	}
	case 'g':
	{
		if (g_params.gravity[1] != 0.0f)
			g_params.gravity[1] = 0.0f;
		else
			g_params.gravity[1] = -9.8f;

		break;
	}
	case '-':
	{
		if (g_params.numPlanes)
			g_params.numPlanes--;

		break;
	}
	case ';':
	{
		g_debug = !g_debug;
		break;
	}
	case 13:
	{
		g_scene = g_selectedScene;
		Init(g_scene);
		break;
	}
	case 27:
	{
		// return quit = true
		return true;
	}
	};

	g_scenes[g_scene]->KeyDown(key);

	return false;
}

void InputKeyboardUp(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
	case 's':
	{
		g_camVel.z = 0.0f;
		break;
	}
	case 'a':
	case 'd':
	{
		g_camVel.x = 0.0f;
		break;
	}
	case 'q':
	case 'z':
	{
		g_camVel.y = 0.0f;
		break;
	}
	};
}

void MouseFunc(int b, int state, int x, int y)
{
	switch (state)
	{
	case SDL_RELEASED:
	{
		g_lastx = x;
		g_lasty = y;
		g_lastb = -1;

		break;
	}
	case SDL_PRESSED:
	{
		g_lastx = x;
		g_lasty = y;
		g_lastb = b;

		if ((SDL_GetModState() & KMOD_LSHIFT) && g_lastb == SDL_BUTTON_LEFT)
		{
			// record that we need to update the picked particle
			g_mousePicked = true;
		}
		break;
	}
	};
}

void MousePassiveMotionFunc(int x, int y)
{
	g_lastx = x;
	g_lasty = y;
}

void MouseMotionFunc(unsigned state, int x, int y)
{
	float dx = float(x - g_lastx);
	float dy = float(y - g_lasty);

	g_lastx = x;
	g_lasty = y;

	if (state & SDL_BUTTON_RMASK)
	{
		const float kSensitivity = DegToRad(0.1f);
		const float kMaxDelta = FLT_MAX;

		g_camAngle.x -= Clamp(dx*kSensitivity, -kMaxDelta, kMaxDelta);
		g_camAngle.y -= Clamp(dy*kSensitivity, -kMaxDelta, kMaxDelta);
	}
}

// returns the new scene if one is selected
int DoUI()
{
	// gui may set a new scene
	int newScene = -1;

	if (g_showHelp)
	{
		const int numParticles = NvFlexGetActiveCount(g_solver);		
		const int numDiffuse = g_buffers->diffuseCount[0];

		int x = g_screenWidth - 200;
		int y = g_screenHeight - 23;

		// imgui
		unsigned char button = 0;
		if (g_lastb == SDL_BUTTON_LEFT)
			button = IMGUI_MBUT_LEFT;
		else if (g_lastb == SDL_BUTTON_RIGHT)
			button = IMGUI_MBUT_RIGHT;

		imguiBeginFrame(g_lastx, g_screenHeight - g_lasty, button, 0);

		x += 180;

		int fontHeight = 13;

		if (1)
		{
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Frame: %d", g_frame); y -= fontHeight * 2;

			if (!g_ffmpeg)
			{
				DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Frame Time: %.2fms", g_realdt*1000.0f); y -= fontHeight * 2;

				// If detailed profiling is enabled, then these timers will contain the overhead of the detail timers, so we won't display them.
				if (!g_profile)
				{
					DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Sim Time (CPU): %.2fms", g_updateTime*1000.0f); y -= fontHeight;
					DrawImguiString(x, y, Vec3(0.97f, 0.59f, 0.27f), IMGUI_ALIGN_RIGHT, "Sim Latency (GPU): %.2fms", g_simLatency); y -= fontHeight * 2;

					BenchmarkUpdateGraph();
				}
				else
				{
					y -= fontHeight * 3;
				}
			}

			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Particle Count: %d", numParticles); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Diffuse Count: %d", numDiffuse); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Rigid Count: %d", g_buffers->rigidOffsets.size() > 0 ? g_buffers->rigidOffsets.size() - 1 : 0); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Spring Count: %d", g_buffers->springLengths.size()); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Num Substeps: %d", g_numSubsteps); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Num Iterations: %d", g_params.numIterations); y -= fontHeight * 2;

			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Device: %s", g_deviceName); y -= fontHeight * 2;
		}

		if (g_profile)
		{
			DrawImguiString(x, y, Vec3(0.97f, 0.59f, 0.27f), IMGUI_ALIGN_RIGHT, "Total GPU Sim Latency: %.2fms", g_timers.total); y -= fontHeight * 2;

			DrawImguiString(x, y, Vec3(0.0f, 1.0f, 0.0f), IMGUI_ALIGN_RIGHT, "GPU Latencies"); y -= fontHeight;

			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Predict: %.2fms", g_timers.predict); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Create Cell Indices: %.2fms", g_timers.createCellIndices); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Sort Cell Indices: %.2fms", g_timers.sortCellIndices); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Reorder: %.2fms", g_timers.reorder); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "CreateGrid: %.2fms", g_timers.createGrid); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Collide Particles: %.2fms", g_timers.collideParticles); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Collide Shapes: %.2fms", g_timers.collideShapes); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Collide Triangles: %.2fms", g_timers.collideTriangles); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Calculate Density: %.2fms", g_timers.calculateDensity); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Solve Densities: %.2fms", g_timers.solveDensities); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Solve Velocities: %.2fms", g_timers.solveVelocities); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Solve Rigids: %.2fms", g_timers.solveShapes); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Solve Springs: %.2fms", g_timers.solveSprings); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Solve Inflatables: %.2fms", g_timers.solveInflatables); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Solve Contacts: %.2fms", g_timers.solveContacts); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Apply Deltas: %.2fms", g_timers.applyDeltas); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Finalize: %.2fms", g_timers.finalize); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Update Triangles: %.2fms", g_timers.updateTriangles); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Update Normals: %.2fms", g_timers.updateNormals); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Update Bounds: %.2fms", g_timers.updateBounds); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Calculate Anisotropy: %.2fms", g_timers.calculateAnisotropy); y -= fontHeight;
			DrawImguiString(x, y, Vec3(1.0f), IMGUI_ALIGN_RIGHT, "Update Diffuse: %.2fms", g_timers.updateDiffuse); y -= fontHeight * 2;
		}

		x -= 180;

		int uiOffset = 250;
		int uiBorder = 20;
		int uiWidth = 200;
		int uiHeight = g_screenHeight - uiOffset - uiBorder * 3;
		int uiLeft = uiBorder;

		if (g_tweakPanel)
			imguiBeginScrollArea("Scene", uiLeft, g_screenHeight - uiBorder - uiOffset, uiWidth, uiOffset, &g_levelScroll);
		else
			imguiBeginScrollArea("Scene", uiLeft, uiBorder, uiWidth, g_screenHeight - uiBorder - uiBorder, &g_levelScroll);

		for (int i = 0; i < int(g_scenes.size()); ++i)
		{
			unsigned int color = g_scene == i ? imguiRGBA(255, 151, 61, 255) : imguiRGBA(255, 255, 255, 200);
			if (imguiItem(g_scenes[i]->GetName(), true, color)) // , i == g_selectedScene))
			{
				newScene = i;
			}
		}

		imguiEndScrollArea();

		if (g_tweakPanel)
		{
			static int scroll = 0;

			imguiBeginScrollArea("Options", uiLeft, g_screenHeight - uiBorder - uiHeight - uiOffset - uiBorder, uiWidth, uiHeight, &scroll);
			imguiSeparatorLine();

			// global options
			imguiLabel("Global");

			if (imguiCheck("Pause", g_pause))
				g_pause = !g_pause;

			imguiSeparatorLine();

			if (imguiCheck("Wireframe", g_wireframe))
				g_wireframe = !g_wireframe;

			if (imguiCheck("Draw Points", g_drawPoints))
				g_drawPoints = !g_drawPoints;

			if (imguiCheck("Draw Fluid", g_drawEllipsoids))
				g_drawEllipsoids = !g_drawEllipsoids;

			if (imguiCheck("Draw Mesh", g_drawMesh))
			{
				g_drawMesh = !g_drawMesh;
			}

			if (imguiCheck("Draw Basis", g_drawBases))
				g_drawBases = !g_drawBases;

			if (imguiCheck("Draw Springs", bool(g_drawSprings != 0)))
				g_drawSprings = (g_drawSprings) ? 0 : 1;

			if (imguiCheck("Draw Contacts", g_drawContacts))
				g_drawContacts = !g_drawContacts;

			imguiSeparatorLine();

			// scene options
			g_scenes[g_scene]->DoGui();

		if (imguiButton("Reset Scene"))
			g_resetScene = true;

			imguiSeparatorLine();

			float n = float(g_numSubsteps);
			if (imguiSlider("Num Substeps", &n, 1, 10, 1))
				g_numSubsteps = int(n);

			n = float(g_params.numIterations);
			if (imguiSlider("Num Iterations", &n, 1, 20, 1))
				g_params.numIterations = int(n);

			imguiSeparatorLine();
			imguiSlider("Gravity X", &g_params.gravity[0], -50.0f, 50.0f, 1.0f);
			imguiSlider("Gravity Y", &g_params.gravity[1], -50.0f, 50.0f, 1.0f);
			imguiSlider("Gravity Z", &g_params.gravity[2], -50.0f, 50.0f, 1.0f);

			imguiSeparatorLine();
			imguiSlider("Radius", &g_params.radius, 0.01f, 0.5f, 0.01f);
			imguiSlider("Solid Radius", &g_params.solidRestDistance, 0.0f, 0.5f, 0.001f);
			imguiSlider("Fluid Radius", &g_params.fluidRestDistance, 0.0f, 0.5f, 0.001f);

			// common params
			imguiSeparatorLine();
			imguiSlider("Dynamic Friction", &g_params.dynamicFriction, 0.0f, 1.0f, 0.01f);
			imguiSlider("Static Friction", &g_params.staticFriction, 0.0f, 1.0f, 0.01f);
			imguiSlider("Particle Friction", &g_params.particleFriction, 0.0f, 1.0f, 0.01f);
			imguiSlider("Restitution", &g_params.restitution, 0.0f, 1.0f, 0.01f);
			imguiSlider("SleepThreshold", &g_params.sleepThreshold, 0.0f, 1.0f, 0.01f);
			imguiSlider("Shock Propagation", &g_params.shockPropagation, 0.0f, 10.0f, 0.01f);
			imguiSlider("Damping", &g_params.damping, 0.0f, 10.0f, 0.01f);
			imguiSlider("Dissipation", &g_params.dissipation, 0.0f, 0.01f, 0.0001f);
			imguiSlider("SOR", &g_params.relaxationFactor, 0.0f, 5.0f, 0.01f);

			imguiSlider("Collision Distance", &g_params.collisionDistance, 0.0f, 0.5f, 0.001f);
			imguiSlider("Collision Margin", &g_params.shapeCollisionMargin, 0.0f, 5.0f, 0.01f);

			// cloth params
			imguiSeparatorLine();
			imguiSlider("Wind", &g_windStrength, -1.0f, 1.0f, 0.01f);
			imguiSlider("Drag", &g_params.drag, 0.0f, 1.0f, 0.01f);
			imguiSlider("Lift", &g_params.lift, 0.0f, 1.0f, 0.01f);
			imguiSeparatorLine();

			// fluid params
			imguiSlider("Adhesion", &g_params.adhesion, 0.0f, 10.0f, 0.01f);
			imguiSlider("Cohesion", &g_params.cohesion, 0.0f, 0.2f, 0.0001f);
			imguiSlider("Surface Tension", &g_params.surfaceTension, 0.0f, 50.0f, 0.01f);
			imguiSlider("Viscosity", &g_params.viscosity, 0.0f, 120.0f, 0.01f);
			imguiSlider("Vorticicty Confinement", &g_params.vorticityConfinement, 0.0f, 120.0f, 0.1f);
			imguiSlider("Solid Pressure", &g_params.solidPressure, 0.0f, 1.0f, 0.01f);
			imguiSlider("Surface Drag", &g_params.freeSurfaceDrag, 0.0f, 1.0f, 0.01f);
			imguiSlider("Buoyancy", &g_params.buoyancy, -1.0f, 1.0f, 0.01f);

			imguiSeparatorLine();
			imguiSlider("Anisotropy Scale", &g_params.anisotropyScale, 0.0f, 30.0f, 0.01f);
			imguiSlider("Smoothing", &g_params.smoothing, 0.0f, 1.0f, 0.01f);

			// diffuse params
			imguiSeparatorLine();
			imguiSlider("Diffuse Threshold", &g_params.diffuseThreshold, 0.0f, 1000.0f, 1.0f);
			imguiSlider("Diffuse Buoyancy", &g_params.diffuseBuoyancy, 0.0f, 2.0f, 0.01f);
			imguiSlider("Diffuse Drag", &g_params.diffuseDrag, 0.0f, 2.0f, 0.01f);
			imguiSlider("Diffuse Scale", &g_diffuseScale, 0.0f, 1.5f, 0.01f);
			imguiSlider("Diffuse Alpha", &g_diffuseColor.w, 0.0f, 3.0f, 0.01f);
			imguiSlider("Diffuse Inscatter", &g_diffuseInscatter, 0.0f, 2.0f, 0.01f);
			imguiSlider("Diffuse Outscatter", &g_diffuseOutscatter, 0.0f, 2.0f, 0.01f);
			imguiSlider("Diffuse Motion Blur", &g_diffuseMotionScale, 0.0f, 5.0f, 0.1f);

			n = float(g_params.diffuseBallistic);
			if (imguiSlider("Diffuse Ballistic", &n, 1, 40, 1))
				g_params.diffuseBallistic = int(n);

			imguiEndScrollArea();
		}
		imguiEndFrame();

		// kick render commands
		DrawImguiGraph();
	}

	return newScene;
}

inline bool ProcessInput(SDL_Event e) {
	bool quit = false;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_QUIT:
			quit = true;
			break;

		case SDL_KEYDOWN:
			if (e.key.keysym.sym < 256 && (e.key.keysym.mod == KMOD_NONE || (e.key.keysym.mod & KMOD_NUM)))
				quit = InputKeyboardDown(e.key.keysym.sym, 0, 0);
			InputArrowKeysDown(e.key.keysym.sym, 0, 0);
			break;

		case SDL_KEYUP:
			if (e.key.keysym.sym < 256 && (e.key.keysym.mod == 0 || (e.key.keysym.mod & KMOD_NUM)))
				InputKeyboardUp(e.key.keysym.sym, 0, 0);
			InputArrowKeysUp(e.key.keysym.sym, 0, 0);
			break;

		case SDL_MOUSEMOTION:
			if (e.motion.state)
				MouseMotionFunc(e.motion.state, e.motion.x, e.motion.y);
			else
				MousePassiveMotionFunc(e.motion.x, e.motion.y);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			MouseFunc(e.button.button, e.button.state, e.motion.x, e.motion.y);
			break;

		case SDL_WINDOWEVENT:
			if (e.window.windowID == g_windowId)
			{
				if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					ReshapeWindow(e.window.data1, e.window.data2);
			}
			break;

		case SDL_WINDOWEVENT_LEAVE:
			g_camVel = Vec3(0.0f, 0.0f, 0.0f);
			break;
		}
	}
	return quit;
}