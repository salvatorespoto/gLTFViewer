#pragma once

#include "DXUtil.h"

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
	void createDefaultDevice();


private:

	HINSTANCE m_hInstance;		/*< Handle to the application instance */
	HWND m_hWnd;				/*< Handle to the application window */
	UINT m_clientWidth;			/*< Client window width */
	UINT m_clientHeight;		/*< Client window height */

	//// Direct3D
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
};