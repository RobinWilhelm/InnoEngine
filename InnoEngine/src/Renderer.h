#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "BaseTypes.h"
#include "Sprite.h"
#include "imgui.h"

#include <string>

namespace InnoEngine
{
    struct ShaderFormatInfo
    {
        SDL_GPUShaderFormat   Format = SDL_GPU_SHADERFORMAT_INVALID;
        std::filesystem::path SubDirectory;
        std::string_view      EntryPoint;
        std::string_view      FileNameExtension;
    };

    class Window;
    class OrthographicCamera;

    class GPURenderer
    {
        friend class Application;

    public:
        ~GPURenderer();

        [[nodiscard]]
        static auto create( Window* window, AssetManager* assetamanager ) -> std::optional<Own<GPURenderer>>;

        Window*        get_window() const;
        SDL_GPUDevice* get_gpudevice() const;
        bool           has_window();

        void set_camera_matrix( const DXSM::Matrix view_projection );

        ShaderFormatInfo get_needed_shaderformat();
        std::string      add_shaderformat_fileextension( std::string_view shaderName );

        bool enable_vsync( bool enabled );
        void on_synchronize();
        void render();    // process all available rendercommands and send them to the gpu

        void add_sprite( const Sprite& sprite );
        void add_imgui_draw_data(ImDrawData* draw_data);

    private:
        void retrieve_shaderformatinfo();

    private:
        class PipelineProcessor;
        Own<PipelineProcessor> m_pipelineProcessor = nullptr;

        SDL_GPUDevice*   m_sdlGPUDevice = nullptr;
        ShaderFormatInfo m_shaderFormat;
        Window*          m_window = nullptr;
        bool             m_vsync  = true;
        // TODO: Create some kind of frameBuffer object
        DXSM::Matrix     m_viewProjection;
    };
}    // namespace InnoEngine
