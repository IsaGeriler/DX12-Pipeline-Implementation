#pragma once

#include "Core.h"
#include "ConstantBuffer.h"
#include "ScreenSpaceTriangle.h"
#include "PSOManager.h"
#include "MyMath.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

// Simplest primitive is a triangle
class Primitive {
public:
	// Vertex and Pixel Shaders
	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;

	// Instance of ScreenSpaceTriangle
	ScreenSpaceTriangle triangle;

	// Instance of Pipeline Stage Object Manager
	PSOManager psos;

	// Constant Buffer
	ConstantBuffer constantBuffer;

	std::vector<ConstantBuffer*> vsConstantBuffers; // Vertex Shader Buffers
	std::vector<ConstantBuffer*> psConstantBuffers; // Pixel Shader Buffers

	void initialize(Core* core) {
		triangle.initialize(core);

		// Init the buffer
		constantBuffer.initialize(core, sizeof(ConstantBuffer2), 1024);

		// Register it to the list
		psConstantBuffers.push_back(&constantBuffer);

		// Load Shaders via Manager
		// Note: Use L"String" for wstring filenames
		core->shaderManager.load("TriangleVS", L"VertexShader.hlsl", "VS", "vs_5_0", VERTEX_SHADER);
		core->shaderManager.load("TrianglePS", L"PixelShader.hlsl", "PS", "ps_5_0", PIXEL_SHADER);

		// Create PSO using the loaded shaders
		psos.createPSO(
			core,
			"Triangle",
			core->shaderManager.getShader("TriangleVS"),
			core->shaderManager.getShader("TrianglePS"),
			triangle.mesh.inputLayoutDesc
		);
	}

	void draw(Core* core, ConstantBuffer2* cb) {
		core->beginRenderPass();

		// Update the GPU memory
		constantBuffer.update(cb, sizeof(ConstantBuffer2));

		// Use apply() to Bind and Advance all buffers automatically
		apply(core);

		psos.bind(core, "Triangle");
		triangle.draw(core);
	}

	void apply(Core* core) {
		// Vertex Shaders start at Root Index 0
		for (int i = 0; i < vsConstantBuffers.size(); i++) {
			core->getCommandList()->SetGraphicsRootConstantBufferView(i, vsConstantBuffers[i]->getGPUAddress());
			vsConstantBuffers[i]->next();
		}

		// Pixel Shaders start at Root Index 1
		for (int i = 0; i < psConstantBuffers.size(); i++) {
			core->getCommandList()->SetGraphicsRootConstantBufferView(i + 1, psConstantBuffers[i]->getGPUAddress());
			psConstantBuffers[i]->next();
		}
	}
};