#pragma once
#include "SDL3/SDL_gpu.h"
#include "Asset.h"

#include <filesystem>

class GPURenderer;

class Shader : public Asset<Shader>
{
public:
    struct ShaderCreateInfo
    {
        uint32_t SamplerCount        = 0;
        uint32_t StorageTextureCount = 0;
        uint32_t StorageBufferCount  = 0;
        uint32_t UniformBufferCount  = 0;
    };

    virtual ~Shader();
    SDL_GPUShader* get_sdlshader() const;

    // Geerbt über Asset
    bool load_from_file( std::filesystem::path fullPath ) override;

    bool create_device_ressources( GPURenderer* pRenderer, const ShaderCreateInfo& createInfo );

private:
    size_t             m_dataSize = 0;
    void*              m_data     = nullptr;
    GPURenderer*       m_renderer = nullptr;
    SDL_GPUShaderStage m_stage;
    SDL_GPUShader*     m_sdlShader = nullptr;
};
