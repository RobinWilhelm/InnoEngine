#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "GPUPipeline.h"
#include "RenderCommandBuffer.h"
#include "BaseTypes.h"

#include <string>
#include <filesystem>
#include <mutex>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <typeindex>

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
        static auto create( Window* pWindow = nullptr ) -> std::optional<Owned<GPURenderer>>;

        template <typename T>
        std::optional<std::shared_ptr<T>> require_pipeline();

        void renderthread_run();
        void renderthread_stop();
        bool process_pipelines();

        Window*        get_window() const;
        SDL_GPUDevice* get_gpudevice() const;

        void set_camera_matrix( const DXSM::Matrix viewProjection );

        ShaderFormatInfo get_needed_shaderformat();
        std::string      add_shaderformat_fileextension( std::string_view shaderName );

        bool has_window();

        bool enable_vsync( bool enabled );

        // not threadsafe
        // if the renderer is running multithreaded, use wait_for_rendering_finished_and_hold_lock() before calling this function
        void submit_pipelines();

    private:
        void do_renderpass();

        void end_frame();
        void retrieve_shaderformatinfo();

    private:
        SDL_GPUDevice*   m_sdlGPUDevice = nullptr;
        ShaderFormatInfo m_shaderFormat;
        Window*          m_window = nullptr;
        bool             m_vsync  = true;

        std::unordered_map<std::type_index, std::shared_ptr<GPUPipeline>> m_loadedPipelines;

        // TODO: Create some kind of frameBuffer object
        DXSM::Matrix m_viewProjection;

        std::vector<GPUPipeline*> m_renderPipelines;
        std::vector<GPUPipeline*> m_computePipelines;

        bool m_needsCopyPass    = false;
        bool m_needsRenderPass  = false;
        bool m_needsComputePass = false;
    };

    template <typename T>
    inline std::optional<std::shared_ptr<T>> GPURenderer::require_pipeline()
    {
        static_assert( std::is_base_of<GPUPipeline, T>::value == true );
        std::type_index tyidx = std::type_index( typeid( T ) );
        auto            it    = m_loadedPipelines.find( tyidx );
        if ( it != m_loadedPipelines.end() ) {
            return std::static_pointer_cast<T>( it->second );
        }

        std::shared_ptr<T> newPipeline = std::make_shared<T>();
        if ( newPipeline->init( this ) == false )
            return std::nullopt;

        m_loadedPipelines.insert( std::pair( tyidx, std::static_pointer_cast<GPUPipeline>( newPipeline ) ) );
        return newPipeline;
    }

}    // namespace InnoEngine
