#pragma once
#include "SDL3/SDL_gpu.h"

#include "Asset.h"
#include "Renderer.h"

#include <filesystem>

namespace InnoEngine
{
    struct ShaderFormatInfo
    {
        SDL_GPUShaderFormat   Format = SDL_GPU_SHADERFORMAT_INVALID;
        std::filesystem::path SubDirectory;
        std::string_view      EntryPoint;
        std::string_view      FileNameExtension;
    };

    class Shader : public Asset<Shader>
    {
        friend class GPURenderer;

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
        bool                  load_from_file( const std::filesystem::path& full_path, std::string_view file_name ) override;
        std::filesystem::path build_path( const std::filesystem::path& folder, std::string_view file_name ) override;

        bool create_device_ressources( GPUDeviceRef device, const ShaderCreateInfo& createInfo );

    private:
        static ShaderFormatInfo ms_shaderFormat;

        GPUDeviceRef       m_device = nullptr;
        size_t             m_dataSize  = 0;
        void*              m_data      = nullptr;
        SDL_GPUShader*     m_sdlShader = nullptr;
        SDL_GPUShaderStage m_stage;
    };

}    // namespace InnoEngine
