#include "Window.h"

// Global pointer to the window
Window* window;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			exit(0);
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			exit(0);
			return 0;
		}
		case WM_KEYDOWN:
		{
			window->keys[(unsigned int)wParam] = true;
			return 0;
		}
		case WM_KEYUP:
		{
			window->keys[(unsigned int)wParam] = false;
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			window->mouseButtons[0] = true;
			return 0;
		}
		case WM_LBUTTONUP:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			window->mouseButtons[0] = false;
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
}

void Window::initialize(unsigned int window_width, unsigned int window_height, std::string window_name, unsigned int window_x, unsigned int window_y) {
	// Create the struct that will hold all the configuration settings for the window.
	WNDCLASSEX wc;

	// Asks the OS for the ID/Handle of the current running application instance. It tells Windows "I am this program".
	hinstance = GetModuleHandle(NULL);

	// CS_HREDRAW | CS_VREDRAW: Tells Windows to redraw the entire window if the user changes the Horizontal or Vertical size.
	// This ensures your graphics don't get stretched or distorted during resizing.
	// CS_OWNDC: Allocates a unique Device Context (DC) for this window. This is critical for graphics applications.
	name = window_name;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	// lpfn: "Long Pointer to Function".
	// WndProc: Handles events like "Mouse Clicked", "Key Pressed", or "Window Closed".
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinstance;

	// hIcon: Sets the icon displayed in the taskbar. IDI_WINLOGO uses the default Windows logo.
	// hCursor: Sets the mouse cursor to the standard arrow (IDC_ARROW).
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	// Sets the background color of the window to Black 
	// (if not, window might flash white for a split second before your DirectX engine starts rendering).
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	// Tells Windows: "We don't want a File/Edit/View menu bar at the top." 
	// Games usually render their own UI, so they don't want the standard Windows menu.
	wc.lpszMenuName = NULL;

	// Modern Windows uses Unicode (Wide Characters) internally.
	std::wstring wname = std::wstring(name.begin(), name.end());
	wc.lpszClassName = wname.c_str();

	// cbSize: You must tell Windows the size of the struct. This allows Windows to detect which version of the API you are using.
	wc.cbSize = sizeof(WNDCLASSEX);

	// This submits the blueprint to the OS.
	RegisterClassEx(&wc);

	// Set window width and height
	width = window_width;
	height = window_height;
	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

	// Create the window
	hwnd = CreateWindowEx(WS_EX_APPWINDOW, wname.c_str(), wname.c_str(), style, window_x, window_y, width, height, NULL, NULL, hinstance, this);
	window = this;
}

void Window::updateMouse(int x, int y) {
	mousex = x;
	mousey = y;
}

void Window::processMessages() {
	// MSG: This is a struct provided by Windows to hold details about an event (e.g., which key was pressed, where the mouse is).
	// ZeroMemory: Clears any garbage data from the variable. It's a safety habit, though PeekMessage will overwrite this data anyway.
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	// Unlike standard apps which use GetMessage() (which puts the CPU to sleep until an event happens), PeekMessage checks the queue
	// and returns immediately. If there is a message, it returns true. If the queue is empty, it returns false.
	// PM_REMOVE: This tells the OS, "I have read this message, please delete it from the queue so I don't read it again."
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}