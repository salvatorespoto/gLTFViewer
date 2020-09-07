#pragma once

#include "DXUtil.h"

/** Main window application events callback */
LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Gui;
struct Light;
struct MeshConstants;
class Scene;
class Camera;
class Scene;
class SkyBox;
class Renderer;
class GLTFSceneLoader;

constexpr unsigned int DEFAULT_SCREEN_WIDTH = 1280;
constexpr unsigned int DEFAULT_SCREEN_HEIGHT = 1024;

/** Store some info about the application state */
struct AppState
{
	unsigned int currentScreenWidth = DEFAULT_SCREEN_WIDTH;
	unsigned int currentScreenHeight = DEFAULT_SCREEN_HEIGHT;

	bool isAppPaused = false;
	bool isResizingWindow = false;
	bool isAppFullscreen = false;
	bool isAppWindowed = false;
	bool isAppMinimized = false;
	bool isExitTriggered = false;
	bool isOpenGLTFPressed = false;
	std::string gltfFileLoaded;
	bool doRecompileShader = false;
	int currentDisplayMode = 0;
	std::map<unsigned int, MeshConstants> meshConstants;
	std::map<unsigned int, Light> lights;
};

/** The 3D viewer application */
class ViewerApp
{
public:
	ViewerApp(HINSTANCE m_hInstance);
	~ViewerApp();
	int Run();
	void SetFullScreen(bool fullScreen);
	
	/** Message procedure called from the window callback */
	virtual LRESULT WndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	void InitWindow();
	void InitWIC();
	void InitRenderer();	
	void InitCamera();
	void InitScene();
	void InitGui();
	void UpdateScene();
	float GetScreenAspectRatio();

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

	std::unique_ptr<Gui> m_gui;
	std::unique_ptr<Camera> m_camera;
	std::shared_ptr<Scene> m_scene;
	std::unique_ptr<SkyBox> m_skyBox;
	std::shared_ptr<Renderer> m_renderer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_cubeMapTexture;

	float m_mouseSensitivity = 0.25f;
	float m_cameraStep = 0.05f;

	int m_lastMousePosX;
	int m_lastMousePosY;
	UINT m_clientWidth;
	UINT m_clientHeight;
	DXGI_MODE_DESC m_fullScreenMode;

	std::unique_ptr<GLTFSceneLoader> m_gltfLoader;

private:
	AppState m_appState;
	HINSTANCE m_hInstance;		
	HWND m_hWnd;	/*< Handle to the main window application */
};