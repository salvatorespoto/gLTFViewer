#pragma once

#include "DXUtil.h"
#include "FrameContext.h"
#include "Buffers.h"
#include "SwapChain.h"
#include "World.h"
#include "Camera.h"
#include "GUI.h"

/** Windows app events callback */
LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class AssetsManager;

/* Store some info about the app current state */
struct AppState
{
	bool isAppPaused = false;
	bool isResizingWindow = false;
	bool isAppFullscreen = false;
	bool isAppWindowed = false;
	bool isAppMinimized = false;
	bool doRecompileShader = false;
	int currentDisplayMode = 0;
	unsigned int currentScreenWidth = 1280;
	unsigned int currentScreenHeight = 1024;
	float meshRotationX = 0.0f, meshRotationY = 0.0f, meshRotationZ = 0.0f;
};

/** The 3D viewer application */
class ViewerApp
{
public:

	/** 
	  * Constructor
	  * @param m_hInstance the handle to the window application
	  */
	ViewerApp(HINSTANCE m_hInstance);
	~ViewerApp();

	void init();
	int run();
	void Update();
	void draw();
	void SetFullScreen(bool fullScreen);
	float getScreenAspectRatio();
	void InitWIC();

	// Called from the window callback
	virtual LRESULT wndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Event handlers
	virtual void OnEnterSizeMove();
	virtual void OnExitSizeMove();
	virtual void OnAppMinimized();
	virtual void OnResize(UINT width, UINT height);
	virtual void onMouseMove(WPARAM btnState, int x, int y);
	virtual void onMouseDown(WPARAM btnState, int x, int y);
	virtual void onMouseUp(WPARAM btnState, int x, int y);
	virtual void onKeyDown(WPARAM btnState);
	std::shared_ptr<AssetsManager> m_assetsManager;

protected:
	void InitWindow();
	void showWindow();
	void LoadWorld();
	void UpdateCamera();
	void onDestroy();


	//void resizeSwapChain(int width, int height);
	//void onFullScreenSwitch();
private:
	HINSTANCE m_hInstance;		/*< Handle to the window application */
	HWND m_hWnd;				
	UINT m_clientWidth;			
	UINT m_clientHeight;	
	float m_mouseSensitivity = 0.25f;
	int m_lastMousePosX;
	int m_lastMousePosY;

	BOOL m_fullScreen = false;
	DXGI_MODE_DESC m_fullScreenMode;

	AppState m_appState;

	std::shared_ptr<Renderer> m_renderer;

	//std::vector<DXGI_MODE_DESC> m_displayModes;
	Gui m_gui;
	std::unique_ptr<Camera> m_camera;
	World m_world;
	
	
};