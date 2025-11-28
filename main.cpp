#include "Core.h"
#include "MyMath.h"
#include "PSOManager.h"
#include "Window.h"

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

		// Create a vertex buffer
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

class ConstantBuffer {
public:
	ID3D12Resource* constantBuffer;
	unsigned char* buffer;
	unsigned int cbSizeInBytes;

	void initialize(Core* core, unsigned int sizeInBytes, int frames) {
		cbSizeInBytes = (sizeInBytes + 255) & ~255;

		HRESULT hr;
		D3D12_HEAP_PROPERTIES heapprops = {};
		heapprops.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapprops.CreationNodeMask = 1;
		heapprops.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC cbDesc = {};
		cbDesc.Width = cbSizeInBytes * frames;
		cbDesc.Height = 1;
		cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbDesc.DepthOrArraySize = 1;
		cbDesc.MipLevels = 1;
		cbDesc.SampleDesc.Count = 1;
		cbDesc.SampleDesc.Quality = 0;
		cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		hr = core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource), (void**)&constantBuffer);
		hr = constantBuffer->Map(0, NULL, (void**)&buffer);
	}

	void update(void* data, unsigned int sizeInBytes, int frame) const {
		memcpy(buffer + (frame * cbSizeInBytes), data, sizeInBytes);
	}

	D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress(int frame) {
		return (constantBuffer->GetGPUVirtualAddress() + (frame * cbSizeInBytes));
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

	std::string readShader(std::string filename) {
		std::ifstream file(filename);
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	void initialize(Core* core) {
		triangle.initialize(core);
		constantBuffer.initialize(core, sizeof(ConstantBuffer1), 2);
		compile(core);
	}

	void compile(Core* core) {
		ID3DBlob* status;

		// Compile vertex shader
		std::string vertexShaderStr = readShader("VertexShader.hlsl");
		HRESULT hr = D3DCompile(vertexShaderStr.c_str(), strlen(vertexShaderStr.c_str()), NULL, NULL, NULL, "VS", "vs_5_0", 0, 0, &vertexShader, &status);

		// Compile pixel shader
		std::string pixelShaderStr = readShader("PixelShader.hlsl");
		hr = D3DCompile(pixelShaderStr.c_str(), strlen(pixelShaderStr.c_str()), NULL, NULL, NULL, "PS", "ps_5_0", 0, 0, &pixelShader, &status);

		// Check if shaders compiled
		if (FAILED(hr)) {
			if (status) {
				// Print the error to the Visual Studio Output window
				OutputDebugStringA((char*)status->GetBufferPointer());
				status->Release();
			}
			return;
		}

		// Create pipeline stage
		psos.createPSO(core, "Triangle", vertexShader, pixelShader, triangle.mesh.inputLayoutDesc);
	}

	void draw(Core* core, ConstantBuffer1* cb) {
		core->beginRenderPass();

		// Update the GPU memory
		constantBuffer.update(cb, sizeof(ConstantBuffer1), core->frameIndex());

		// Bind Constant Buffer View to Root Signature index
		core->getCommandList()->SetGraphicsRootConstantBufferView(1, constantBuffer.getGPUAddress(core->frameIndex()));

		psos.bind(core, "Triangle");
		triangle.draw(core);
	}
};

// C28251 warning -> int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ int nCmdShow) {
	Window window;
	Core core;
	Primitive primitive;
	GamesEngineeringBase::Timer timer;
	
	window.initialize(1024, 1024, "My Window");
	core.initialize(window.hwnd, 1024, 1024);
	primitive.initialize(&core);

	ConstantBuffer1 constBufferCPU;
	constBufferCPU.time = 0;
	
	while (true) {
		float dt = timer.dt();
		constBufferCPU.time += dt;

		if (window.keys[VK_ESCAPE] == 1) break;
		core.beginFrame();
		window.processMessages();
		primitive.draw(&core, &constBufferCPU);
		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}