#pragma once

// Extracts the X-coordinate.
#define WINDOW_GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))  // Low-order 16 bits of the LPARAM value

// Extracts the Y-coordinate.
#define WINDOW_GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))  // High-order 16 bits of the LPARAM value.

#include <string>
#include <Windows.h>

class Window {
public:
	// Attributes
	HWND hwnd;					 // Handle to a window
	HINSTANCE hinstance;		 // Handle to an instance/a module

	std::string name;			 // Window name
	unsigned int width, height;  // Window width and height

	// Store key presses and mouse position
	bool keys[256];
	bool mouseButtons[3];
	int mousex;
	int mousey;

	// Methods
	void initialize(unsigned int width, unsigned int height, std::string name, unsigned int window_x = 0, unsigned int window_y = 0);
	void updateMouse(int x, int y);
	void processMessages();
};