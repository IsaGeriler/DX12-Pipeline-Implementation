#include "Mesh.h"

void Mesh::initialize(Core* core, void* vertices, int vertexSizeInBytes, int numVertices) {
	// Specify vertex buffer will be in GPU memory heap
	D3D12_HEAP_PROPERTIES heapprops = {};
	heapprops.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapprops.CreationNodeMask = 1;
	heapprops.VisibleNodeMask = 1;

	// Create a vertex buffer on heap
	D3D12_RESOURCE_DESC vbDesc = {};
	vbDesc.Width = numVertices * vertexSizeInBytes;
	vbDesc.Height = 1;
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.SampleDesc.Count = 1;
	vbDesc.SampleDesc.Quality = 0;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// Allocate memory
	core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&vertexBuffer));

	// Copy vertices using our helper function
	core->uploadResource(vertexBuffer, vertices, numVertices * vertexSizeInBytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Fill in view in helper function
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = vertexSizeInBytes;
	vbView.SizeInBytes = numVertices * vertexSizeInBytes;

	// Fill in Layout
	inputLayout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputLayout[1] = { "COLOUR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputLayoutDesc.NumElements = 2;
	inputLayoutDesc.pInputElementDescs = inputLayout;
}

void Mesh::draw(Core* core) const {
	core->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	core->getCommandList()->IASetVertexBuffers(0, 1, &vbView);
	core->getCommandList()->DrawInstanced(3, 1, 0, 0);
}