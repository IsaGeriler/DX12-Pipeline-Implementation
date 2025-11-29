#pragma once

#include <map>
#include <string>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

enum ShaderType { VERTEX_SHADER, PIXEL_SHADER };

struct Shader {
	ID3DBlob* blob;
	ShaderType type;
};

class ShaderManager {
public:
	std::map<std::string, Shader> shaders;

    ~ShaderManager() {
        for (auto& pair : shaders) {
            // pair.first = name (string) && pair.second = Shader (Shader)
            if (pair.second.blob) {
                pair.second.blob->Release();
            }
        }
    }

    ID3DBlob* getShader(std::string name) {
        if (shaders.find(name) != shaders.end()) return shaders[name].blob;
        return nullptr;
    }

    void load(std::string name, std::wstring filename, std::string entryPoint, std::string profile, ShaderType type) {
        ID3DBlob* blob = nullptr;
        ID3DBlob* error = nullptr;
        HRESULT hr;

        // Try to load pre-compiled CSO file first
        std::wstring csoName = filename + L".cso";
        hr = D3DReadFileToBlob(csoName.c_str(), &blob);

        if (SUCCEEDED(hr)) {
            shaders[name] = { blob, type };
            return;
        }

        // If CSO file is missing, compile from HLSL file
        hr = D3DCompileFromFile(
            filename.c_str(),    // File path (L"VertexShader.hlsl")
            nullptr, nullptr,    // Macros and Includes
            entryPoint.c_str(),  // Function name (e.g., "VS" or "main")
            profile.c_str(),     // Target (e.g., "vs_5_0")
            0, 0,                // Flags
            &blob, &error        // Outputs
        );

        if (FAILED(hr)) {
            if (error) {
                OutputDebugStringA((char*)error->GetBufferPointer());
                error->Release();
            }
            return;
        }

        // Save to CSO so next run is faster
        D3DWriteBlobToFile(blob, csoName.c_str(), FALSE);

        // Store in map
        shaders[name] = { blob, type };
    }
};