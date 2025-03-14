#pragma once
#include "SDL3/SDL_gpu.h"

#include "CoreAPI.h"
#include "BaseTypes.h"
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
        friend class AssetRepository<Shader>;

        Shader() :
            m_device( CoreAPI::get_gpurenderer()->get_gpudevice() ) { };

    public:
        virtual ~Shader();

        SDL_GPUShader* get_sdlshader() const;

    private:
        // Geerbt �ber Asset
        Result                load_asset( const std::filesystem::path& full_path ) override;
        std::filesystem::path build_path( const std::filesystem::path& folder, std::string_view file_name ) override;

    private:
        static ShaderFormatInfo ms_shaderFormat;

        GPUDeviceRef       m_device    = nullptr;
        SDL_GPUShader*     m_sdlShader = nullptr;
        SDL_GPUShaderStage m_stage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX;
    };

}    // namespace InnoEngine
