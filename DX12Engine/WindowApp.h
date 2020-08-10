#pragma once

#include "DXUtil.h"
#include "FrameContext.h"
#include "Buffers.h"
#include "World.h"
#include "Camera.h"

/** Windows app events callback */
LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/** The base class for a Direct3D application */
class WindowApp
{

public:

	/** Constructor, m_hInstance is the handle to the application, passed from winMain */
	WindowApp(HINSTANCE m_hInstance);

	/** Destructor */
	~WindowApp();

	/** Called from the window callback */
	virtual LRESULT wndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/** Initialize the windows app */
	void init();

	/** Implements the message loop */
	int run();

	/** Called each frame to update the app */
	void update();

	/** Draw the scene */
	void draw();


	// Event handlers
	virtual void onResize();
	virtual void onMouseMove(WPARAM btnState, int x, int y);
	virtual void onMouseDown(WPARAM btnState, int x, int y);
	virtual void onMouseUp(WPARAM btnState, int x, int y);
	virtual void onKeyDown(WPARAM btnState);

	/** Return aspect ratio : screen width / screen height */
	float getScreenAspectRatio();

protected:

	/** Create the window associated with this app*/
	void initWindow();

	/** Show the window associated with this app*/
	void showWindow();

	/** On destroy */
	void onDestroy();

	// D3D related functions
	void enableDebugLayer();
	void createDXGIFactory();
	void createDefaultDevice();
	
	void createCommandObjects();
	
	void createFence();
	
	void createDescriptorHeaps();
	//void createFrameContexts();
	
	void setBackBufferFormat();
	void createSwapChain();
	void createRenderTargetViews();
	
	void createDepthStencilBuffer();
	void createDepthStencilBufferView();
	
	void createConstantBuffers();
	void createConstantBuffersView();
	void updateConstantBuffers();

	void checkMultisampling();

	void setUpViewport();
	void setUpScissorRect();
	
	void createRootSignature();
	void compileShaders();
	void createPipelineStateObject();

	void flushCmdQueue();
	
	void loadWorld();


	ID3D12Resource* getCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilView() const;
	
private:

	//// Window attributes

	HINSTANCE m_hInstance;		/*< Handle to the application instance */
	HWND m_hWnd;				/*< Handle to the application window */
	UINT m_clientWidth;			/*< Client window width */
	UINT m_clientHeight;		/*< Client window height */


	//// Direct3D attributes
	
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgiFactory;

	// Command objects
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

	// Fence
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_currentFenceValue = 0;

	// Swap chain
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffers[2];
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT m_currentBackBuffer = 0; // The id of the current back buffer in the swap chain

	// Depth stencil
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;
	DXGI_FORMAT m_depthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;

	// Descriptors
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_srv_DescriptorHeap;
	UINT m_descriptor_CBV_SRV_size = 0;
	UINT m_descriptor_RTV_size = 0;
	UINT m_descriptor_DSV_size = 0;

	// Constant buffers
	std::unique_ptr<UploadBuffer<PassConstants>> m_passConstantBuffer;

	// Root signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	// Pipeline state object
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;

	//std::vector<FrameContext2> m_frameContexts;
	UINT m_frameInFlight = 3;
	
	UINT m_MSAASampleCount = 1;
	UINT m_MSAAQualityLevel = 0;
	
	D3D12_VIEWPORT m_viewPort;
	D3D12_RECT m_scissorRect;

	World m_world;
	std::unique_ptr<Camera> m_camera;

	float m_mouseSensitivity = 0.25f;
	int m_lastMousePosX;
	int m_lastMousePosY;
};