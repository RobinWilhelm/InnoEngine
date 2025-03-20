#pragma once
#include "InnoEngine/IE_Assert.h"

namespace InnoEngine
{
    class Application;
    class AssetManager;
    class GPURenderer;
    class Camera;
    class Profiler;
    class InputSystem;

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
            IE_ASSERT( get_instance().m_App != nullptr );
            return get_instance().m_App;
        }

        static AssetManager* get_assetmanager()
        {
            IE_ASSERT( get_instance().m_AssetManager != nullptr );
            return get_instance().m_AssetManager;
        }

        static GPURenderer* get_gpurenderer()
        {
            IE_ASSERT( get_instance().m_Renderer != nullptr );
            return get_instance().m_Renderer;
        }

        static Camera* get_camera()
        {
            IE_ASSERT( get_instance().m_Camera != nullptr );
            return get_instance().m_Camera;
        }

        static Profiler* get_profiler()
        {
            IE_ASSERT( get_instance().m_Profiler != nullptr );
            return get_instance().m_Profiler;
        }

        static InputSystem* get_inputsystem()
        {
            IE_ASSERT( get_instance().m_Input != nullptr );
            return get_instance().m_Input;
        }

    private:
        Application*  m_App          = nullptr;
        AssetManager* m_AssetManager = nullptr;
        GPURenderer*  m_Renderer     = nullptr;
        Camera*       m_Camera       = nullptr;
        Profiler*     m_Profiler     = nullptr;
        InputSystem*  m_Input        = nullptr;
    };

}    // namespace InnoEngine
