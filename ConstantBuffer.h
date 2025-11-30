#pragma once

#include "Core.h"
#include "MyMath.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

struct alignas(16) ConstantBuffer1 {
	float time;
};

struct alignas(16) ConstantBuffer2 {
	float time;
	float padding[3];
	Vec4 lights[4];
};

struct ConstantBufferVariable {
	std::string name;
	unsigned int offset;  // Byte offset in the struct (e.g., time is 0)
	unsigned int size;    // Size in bytes
};

class ConstantBuffer {
public:
	ID3D12Resource* constantBuffer;
	unsigned char* buffer;

	unsigned int cbSizeInBytes;
	unsigned int maxDrawCalls;
	unsigned int offsetIndex;

	std::map<std::string, ConstantBufferVariable> constantBufferData;

	void initialize(Core* core, unsigned int sizeInBytes, unsigned int _maxDrawCalls = 1024) {
		maxDrawCalls = _maxDrawCalls;
		cbSizeInBytes = (sizeInBytes + 255) & ~255;
		unsigned int cbSizeInBytesAligned = cbSizeInBytes * maxDrawCalls;
		offsetIndex = 0;

		HRESULT hr;
		D3D12_HEAP_PROPERTIES heapprops = {};
		heapprops.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapprops.CreationNodeMask = 1;
		heapprops.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC cbDesc = {};
		cbDesc.Width = cbSizeInBytesAligned;  // Create a huge buffer
		cbDesc.Height = 1;
		cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbDesc.DepthOrArraySize = 1;
		cbDesc.MipLevels = 1;
		cbDesc.SampleDesc.Count = 1;
		cbDesc.SampleDesc.Quality = 0;
		cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
			IID_PPV_ARGS(&constantBuffer));
		constantBuffer->Map(0, NULL, (void**)&buffer);
	}

	void update(void* data, unsigned int sizeInBytes) {
		memcpy(buffer + (offsetIndex * cbSizeInBytes), data, sizeInBytes);
	}

	D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const {
		return (constantBuffer->GetGPUVirtualAddress() + (offsetIndex * cbSizeInBytes));
	}

	void next() {
		offsetIndex++;
		if (offsetIndex >= maxDrawCalls) {
			offsetIndex = 0;
		}
	}
};