#pragma once

#define WINDOW_GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define WINDOW_GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#include <string>
#include <Windows.h>

class Window {
public:
	/* Window Class Attributes */
	HWND hwnd;
	HINSTANCE hinstance;

	std::string name;
	unsigned int width, height;

	// Store key presses and mouse position
	bool keys[256];
	bool mouseButtons[3];
	int mousex;
	int mousey;

	/* Methods */
	void initialize(unsigned int width, unsigned int height, std::string name, unsigned int window_x = 0, unsigned int window_y = 0);
	void updateMouse(int x, int y);
	void processMessages();
};