#define _USE_MATH_DEFINES
#define M_PI 3.14159265358979323846

#include "Core.h"
#include "MyMath.h"
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

// Define a vertex
struct PRIM_VERTEX {
	Vec3 position;
	Colour colour;
};

class Mesh {
public:
	// Create buffer and upload vertices to GPU
	ID3D12Resource* vertexBuffer;

	// Create view member variable
	D3D12_VERTEX_BUFFER_VIEW vbView;

	// Define layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[2];
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;


	void initialize(Core* core, void* vertices, int vertexSizeInBytes, int numVertices) {
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

	void draw(Core* core) const {
		core->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		core->getCommandList()->IASetVertexBuffers(0, 1, &vbView);
		core->getCommandList()->DrawInstanced(3, 1, 0, 0);
	}
};

class ScreenSpaceTriangle {
public:
	PRIM_VERTEX vertices[3];
	Mesh mesh;

	void initialize(Core* core) {
		vertices[0].position = Vec3(0, 1.0f, 0);
		vertices[0].colour = Colour(0, 1.0f, 0);
		vertices[1].position = Vec3(-1.0f, -1.0f, 0);
		vertices[1].colour = Colour(1.0f, 0, 0);
		vertices[2].position = Vec3(1.0f, -1.0f, 0);
		vertices[2].colour = Colour(0, 0, 1.0f);

		mesh.initialize(core, &vertices[0], sizeof(PRIM_VERTEX), 3);
	}

	void draw(Core* core) const {
		mesh.draw(core);
	}
};


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

	void initialize(Core* core, unsigned int sizeInBytes, unsigned int _maxDrawCalls = 1024)
	{
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

/*
	Entry-point function - WinMain
	WinMain Parameters:
	1. hInstance - handle to an instance/a module. The operating system uses this value to identify the EXE when it's loaded in memory.
	2. hPrevInstance - has no meaning. It was used in 16-bit Windows, but is now always zero.
	3. lpCmdLine - contains the command-line arguments as a Unicode string.
	4. nCmdShow - flag that indicates whether the main application window is minimized, maximized, or shown normally.
*/

// C28251 warning -> int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ int nCmdShow) {
	// Define screen dimensions
	float WIDTH = 1024.0f;
	float HEIGHT = 1024.0f;

	Window window;
	Core core;
	Primitive primitive;
	GamesEngineeringBase::Timer timer;
	
	window.initialize(1024, 1024, "My Window");
	core.initialize(window.hwnd, 1024, 1024);
	primitive.initialize(&core);

	// ConstantBuffer1 constBufferCPU;
	ConstantBuffer2 constBufferCPU2;

	// constBufferCPU.time = 0;
	constBufferCPU2.time = 0;
	
	while (true) {
		if (window.keys[VK_ESCAPE] == 1) break;

		float dt = timer.dt();
		// constBufferCPU.time += dt;
		constBufferCPU2.time += dt;

		// Let’s add lights spinning over the triangle
		for (int i = 0; i < 4; i++) {
			float angle = constBufferCPU2.time + (i * M_PI / 2.0f);
			constBufferCPU2.lights[i] = Vec4(WIDTH / 2.0f + (cosf(angle) * (WIDTH * 0.3f)), HEIGHT / 2.0f + (sinf(angle) * (HEIGHT * 0.3f)), 0, 0);
		}
		core.beginFrame();
		window.processMessages();
		primitive.draw(&core, &constBufferCPU2);
		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}