#define _USE_MATH_DEFINES
#define M_PI 3.14159265358979323846

#include "Core.h"
#include "ConstantBuffer.h"
#include "Mesh.h"
#include "MyMath.h"
#include "ScreenSpaceTriangle.h"
#include "Primitive.h"
#include "PSOManager.h"
#include "Window.h"

#include <map>
#include <cmath>
#include <fstream>
#include <sstream>

// Laptops and Nvidia GPUs - Not found by default
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

/*
 *	Entry-point function - WinMain
 *	WinMain Parameters:
 *	1. hInstance - handle to an instance/a module. The operating system uses this value to identify the EXE when it's loaded in memory.
 *	2. hPrevInstance - has no meaning. It was used in 16-bit Windows, but is now always zero.
 *	3. lpCmdLine - contains the command-line arguments as a Unicode string.
 *	4. nCmdShow - flag that indicates whether the main application window is minimized, maximized, or shown normally.
 */

// C28251 warning -> int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ int nCmdShow) {
	unsigned int WIDTH = 1024, HEIGHT = 1024;  // Define screen dimensions

	Window window;
	Core core;
	Primitive primitive;
	GamesEngineeringBase::Timer timer;
	
	window.initialize(WIDTH, HEIGHT, "My Window");
	core.initialize(window.hwnd, WIDTH, HEIGHT);
	primitive.initialize(&core);
	float time = 0.f;
	// ConstantBuffer2 constBufferCPU2;   // Pulsing Triangle -> ConstantBuffer1 constBufferCPU1;
	// constBufferCPU2.time = 0;		  // Pulsing Triangle -> constBufferCPU1.time = 0;
	
	while (true) {
		if (window.keys[VK_ESCAPE] == 1) break;
		float dt = timer.dt();
		time += dt;
		primitive.constantBuffer.update("time", &time);
		// constBufferCPU2.time += dt;  // Pulsing Triangle -> constBufferCPU1.time += dt;

		// Let’s add lights spinning over the triangle
		Vec4 lights[4];
		for (int i = 0; i < 4; i++) {
			float angle = time + (i * M_PI / 2.0f);
			lights[i] = Vec4(
				WIDTH / 2.0f + (cosf(angle) * (WIDTH * 0.3f)),
				HEIGHT / 2.0f + (sinf(angle) * (HEIGHT * 0.3f)), 
				0, 0
			);
		}
		// Update the array in the buffer
		primitive.constantBuffer.update("lights", lights);

		core.beginFrame();
		window.processMessages();

		// Draw (Internal 'apply' handles the GPU address assignment)
		primitive.draw(&core);
		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}