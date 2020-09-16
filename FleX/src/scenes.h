#pragma once

class Scene
{
public:

	Scene(const char* name) : mName(name) {}
	
	virtual void Initialize() = 0;
	virtual void PostInitialize() {}

	virtual void Destroy() {}

	virtual void Export(const char* basename) {}
	
	// update any buffers (all guaranteed to be mapped here)
	virtual void Update() {}	

	// send any changes to flex (all buffers guaranteed to be unmapped here)
	virtual void Sync() {}
	
	virtual void Draw(int pass) {}
	virtual void KeyDown(int key) {}
	virtual void DoGui() {}
	virtual void CenterCamera() {}

	virtual Matrix44 GetBasis() { return Matrix44::kIdentity; }	

	virtual const char* GetName() { return mName; }

	const char* mName;
};

//#include "scenes/clothlayers.h"
//#include "scenes/envcloth.h"
//#include "scenes/flag.h"
//#include "scenes/fluidclothcoupling.h"
#include "scenes/softbody.h"
//#include "scenes/spherecloth.h"
//#include "scenes/tearing.h"
//#include "scenes/occlusion.h"
#include "scenes/rigid_drop.h"
#include "scenes/banana_drop.h"


#include "scenes/wind.h"
#include "scenes/rotate.h"
#include "scenes/drape.h"
#include "scenes/ball.h"
//#include "scenes/bench.h"