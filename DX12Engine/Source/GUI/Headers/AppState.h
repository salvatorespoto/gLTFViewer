#pragma once

#include "Mesh.h"
#include "Light.h"

#include <string>
#include <map>

/** Store some info about the application state */
struct AppState
{
	unsigned int screenWidth;
	unsigned int screenHeight;

	bool isAppPaused = false;
	bool isResizingWindow = false;
	bool isAppFullscreen = false;
	bool isAppWindowed = false;
	bool isAppMinimized = false;
	bool isExitTriggered = false;
	bool isOpenGLTFPressed = false;
	bool showSkyBox = true;
	bool doRecompileShader = false;
	int currentRenderModeMask = 0; // Render modes: 0 render, 1 wireframe, 2 base color, 3 rough map, 4 occlusion map, 5 emissive map 
	int currentDisplayMode = 0;

	std::string gltfFileLoaded;
	std::map<unsigned int, MeshConstants> modelConstants;
	std::map<unsigned int, Light> lights;	// Light 0 is used as "Ambient light", i.e. only the color is considered
};