#pragma once
#include "InnoEngine/IE_Assert.h"

namespace InnoEngine
{
    class Application;
    class AssetManager;
    class GPURenderer;
    class OrthographicCamera;
    class Profiler;

    class CoreAPI
    {
        friend class Application;

    private:
        CoreAPI()  = default;
        ~CoreAPI() = default;

        static CoreAPI& get_instance()
        {
            static CoreAPI coreapi;
            return coreapi;
        }

    public:
        CoreAPI( const CoreAPI& )            = delete;
        CoreAPI& operator=( const CoreAPI& ) = delete;

        static Application* get_application()
        {
            IE_ASSERT( get_instance().m_app != nullptr );
            return get_instance().m_app;
        }

        static AssetManager* get_assetmanager()
        {
            IE_ASSERT( get_instance().m_assetManager != nullptr );
            return get_instance().m_assetManager;
        }

        static GPURenderer* get_gpurenderer()
        {
            IE_ASSERT( get_instance().m_renderer != nullptr );
            return get_instance().m_renderer;
        }

        static OrthographicCamera* get_camera()
        {
            IE_ASSERT( get_instance().m_camera != nullptr );
            return get_instance().m_camera;
        }

        static Profiler* get_profiler()
        {
            IE_ASSERT(get_instance().m_profiler != nullptr);
            return get_instance().m_profiler;
        }

    private:
        Application*        m_app          = nullptr;
        AssetManager*       m_assetManager = nullptr;
        GPURenderer*        m_renderer     = nullptr;
        OrthographicCamera* m_camera       = nullptr;
        Profiler*           m_profiler     = nullptr;
    };

}    // namespace InnoEngine
