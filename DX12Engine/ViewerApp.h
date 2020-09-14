#pragma once

#include "DXUtil.h"

/** Main window application events callback */
LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Gui;
class Camera;
class Scene;
class SkyBox;
class Grid;
class Renderer;
class GLTFSceneLoader;
struct Light;
struct MeshConstants;

static constexpr unsigned int DEFAULT_SCREEN_WIDTH = 1280;
static constexpr unsigned int DEFAULT_SCREEN_HEIGHT = 1024;

/** Store some info about the application state */
struct AppState
{
	unsigned int screenWidth = DEFAULT_SCREEN_WIDTH;
	unsigned int screenHeight = DEFAULT_SCREEN_HEIGHT;

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

/** The 3D viewer application */
class ViewerApp
{
public:
	ViewerApp(const HINSTANCE& m_hInstance);
	~ViewerApp();
	int Run();
	void SetFullScreen(bool fullScreen);
	
	/** Message procedure called from the window callback */
	virtual LRESULT WndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void InitWindow();
	virtual void InitWIC();
	virtual void InitRenderer();
	virtual void InitCamera();
	virtual void InitScene();
	virtual void InitGui();
	virtual void UpdateScene();
	
	// Event handlers
	virtual void OnEnterSizeMove();
	virtual void OnExitSizeMove();
	virtual void OnAppMinimized();
	virtual void OnResize(UINT width, UINT height);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnKeyDown(WPARAM btnState);
	virtual void OnDestroy();
	virtual void OnUpdate();
	virtual void OnDraw();

	float GetScreenAspectRatio();

	std::unique_ptr<Gui> m_gui;
	std::unique_ptr<Camera> m_camera;
	std::shared_ptr<Scene> m_scene;
	std::unique_ptr<SkyBox> m_skyBox;
	std::unique_ptr<Grid> m_grid;
	std::shared_ptr<Renderer> m_renderer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_cubeMapTexture;

	UINT m_clientWidth;
	UINT m_clientHeight;
	DXGI_MODE_DESC m_fullScreenMode;
	std::unique_ptr<GLTFSceneLoader> m_gltfLoader;
	AppState m_appState;
	float m_mouseSensitivity = 0.25f;
	float m_cameraStep = 0.05f;
	int m_lastMousePosX;
	int m_lastMousePosY;

private:
	HINSTANCE m_hInstance;		
	HWND m_hWnd;	// Handle to the main window application
};