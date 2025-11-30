#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "Core.h"

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

class Mesh {
public:
	// Create buffer and upload vertices to GPU
	ID3D12Resource* vertexBuffer;

	// Create view member variable
	D3D12_VERTEX_BUFFER_VIEW vbView;

	// Define layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[2];
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;

	// Methods
	void initialize(Core* core, void* vertices, int vertexSizeInBytes, int numVertices);
	void draw(Core* core) const;
};