#pragma once

class Application;
class AssetManager;
class GPURenderer;
class OrthographicCamera;

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
        return get_instance().m_app;
    }

    static AssetManager* get_assetmanager()
    {
        return get_instance().m_assetManager;
    }

    static GPURenderer* get_gpurenderer()
    {
        return get_instance().m_renderer;
    }

    static OrthographicCamera* get_camera()
    {
        return get_instance().m_camera;
    }

private:
    Application*        m_app          = nullptr;
    AssetManager*       m_assetManager = nullptr;
    GPURenderer*        m_renderer     = nullptr;
    OrthographicCamera* m_camera       = nullptr;
};
