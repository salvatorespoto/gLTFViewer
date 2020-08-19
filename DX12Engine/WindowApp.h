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

struct AppState
{
	bool m_isAppPaused = false;
	bool m_isResizingWindow = false;
	bool m_isAppFullscreen = false;
	bool m_isAppMinimized = false;
};

/** The base class for a Direct3D application */
class WindowApp
{

public:

	/** Constructor, m_hInstance is the handle to the application, passed from winMain */
	WindowApp(HINSTANCE m_hInstance);
	~WindowApp();

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

protected:

	void InitWindow();
	void showWindow();
	void onDestroy();

	
	//void createFrameContexts();

	void LoadWorld();
	void UpdateCamera();

	//void resizeSwapChain(int width, int height);
	//void onFullScreenSwitch();
	
private:

	AppState m_state;

	//// Window attributes

	HINSTANCE m_hInstance;		/*< Handle to the application instance */
	HWND m_hWnd;				/*< Handle to the application window */
	UINT m_clientWidth;			/*< Client window width */
	UINT m_clientHeight;		/*< Client window height */

	BOOL m_fullScreen = false;
	DXGI_MODE_DESC m_fullScreenMode;
	
	BOOL m_isAppPaused = false;
	BOOL m_isResizingWindow = false;
	BOOL m_isAppFullscreen = false;
	BOOL m_isAppMinimized = false;

	std::shared_ptr<Renderer> m_renderer;

	//std::vector<DXGI_MODE_DESC> m_displayModes;

	Gui GUI;

	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdListAlloc;
	//Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

	
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_srv_DescriptorHeap;
	//UINT m_descriptor_CBV_SRV_size = 0;
	//UINT m_descriptor_DSV_size = 0;

	
	//std::unique_ptr<UploadBuffer<PassConstants>> m_passConstantBuffer;

	//std::vector<FrameContext2> m_frameContexts;
	//UINT m_frameInFlight = 3;

	World m_world;
	std::unique_ptr<Camera> m_camera;

	float m_mouseSensitivity = 0.25f;
	int m_lastMousePosX;
	int m_lastMousePosY;
};