#pragma once

#include "Core.h"
#include "ConstantBuffer.h"
#include "ScreenSpaceTriangle.h"
#include "PSOManager.h"
#include "MyMath.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

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

		// Load Shaders via Manager (Note: Use L"String" for wstring filenames)
		core->shaderManager.load("TriangleVS", L"VertexShader.hlsl", "VS", "vs_5_0", VERTEX_SHADER);
		core->shaderManager.load("TrianglePS", L"PixelShader.hlsl", "PS", "ps_5_0", PIXEL_SHADER);

		ID3DBlob* vsBlob = core->shaderManager.getShader("TriangleVS");
		ID3DBlob* psBlob = core->shaderManager.getShader("TrianglePS");

		// Reflect shader and get details (description)
		ID3D12ShaderReflection* reflection;
		D3DReflect(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), IID_PPV_ARGS(&reflection));
		D3D12_SHADER_DESC desc;
		reflection->GetDesc(&desc);

		bool constantBufferFound = false;

		// Iterate over constant buffers
		for (int i = 0; i < desc.ConstantBuffers; i++) {
			// Get details about i’th constant buffer
			ID3D12ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			constantBuffer->GetDesc(&cbDesc);

			ConstantBuffer* buffer = (i == 0) ? &this->constantBuffer : new ConstantBuffer();
			unsigned int totalSize = 0;

			// Iterate over variables in constant buffer
			for (int j = 0; j < cbDesc.Variables; j++) {
				// Fill in details for each variable
				// Keep a running total of size
				ID3D12ShaderReflectionVariable* var = constantBuffer->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC vDesc;
				var->GetDesc(&vDesc);
				ConstantBufferVariable bufferVariable;
				bufferVariable.name = vDesc.Name;
				bufferVariable.offset = vDesc.StartOffset;
				bufferVariable.size = vDesc.Size;

				buffer->constantBufferData.insert({ bufferVariable.name, bufferVariable });
				if (vDesc.StartOffset + vDesc.Size > totalSize) totalSize = vDesc.StartOffset + vDesc.Size;
			}

			// Initialize the buffer with the calculated size
			buffer->name = cbDesc.Name;
			buffer->initialize(core, totalSize);

			// Add to list for binding
			psConstantBuffers.push_back(buffer);

			if (i == 0) constantBufferFound = true;
		}

		reflection->Release();

		if (!constantBufferFound) {
			OutputDebugStringA("WARNING: No Constant Buffers found in shader! Using default.\n");
			// Initialize with dummy size so pointers are valid
			this->constantBuffer.initialize(core, 256);
			psConstantBuffers.push_back(&this->constantBuffer);
		}

		// Create PSO using the loaded shaders
		psos.createPSO(core, "Triangle", vsBlob, psBlob, triangle.mesh.inputLayoutDesc);
	}

	void draw(Core* core) {
		core->beginRenderPass();

		// Use apply() to Bind and Advance all buffers automatically
		apply(core);

		psos.bind(core, "Triangle");
		triangle.draw(core);
	}

	void apply(Core* core) {
		// Bind VS buffers
		for (int i = 0; i < vsConstantBuffers.size(); i++) {
			core->getCommandList()->SetGraphicsRootConstantBufferView(i, vsConstantBuffers[i]->getGPUAddress());
			vsConstantBuffers[i]->next();
		}
		// Bind PS buffers (Offset by 1)
		for (int i = 0; i < psConstantBuffers.size(); i++) {
			core->getCommandList()->SetGraphicsRootConstantBufferView(1 + i, psConstantBuffers[i]->getGPUAddress());
			psConstantBuffers[i]->next();
		}
	}
};