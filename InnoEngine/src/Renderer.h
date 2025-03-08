#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "imgui.h"

#include "BaseTypes.h"

#include <string>
#include <atomic>

namespace InnoEngine
{
    class Window;
    class OrthographicCamera;
    class Sprite;

    // have reference counting only in debug mode so we can check if we forgot do release stuff
    // no idea if this is a good way to do it
#ifdef _DEBUG
    class GPUDeviceRef
    {
        friend class GPURenderer;

        static GPUDeviceRef create( SDL_GPUDevice* device )
        {
            GPUDeviceRef deviceref;
            deviceref.m_device   = device;
            deviceref.m_useCount = new std::atomic_uint32_t( 1 );
            return deviceref;
        }

    public:
        GPUDeviceRef() = default;

        GPUDeviceRef( SDL_GPUDevice* device ) :
            m_device( device )
        {
            // it should be allowed to initialize it with nullptr so this class is easily replacable by a raw pointer
            IE_ASSERT( device == nullptr );
        };

        GPUDeviceRef( const GPUDeviceRef& other )
        {
            m_device   = other.m_device;
            m_useCount = other.m_useCount;
            addref();
        }

        GPUDeviceRef( GPUDeviceRef&& other )
        {
            m_device         = other.m_device;
            m_useCount       = other.m_useCount;
            other.m_device   = nullptr;
            other.m_useCount = nullptr;
        }

        GPUDeviceRef& operator=( const GPUDeviceRef& other )
        {
            if ( this == &other )
                return *this;

            m_device   = other.m_device;
            m_useCount = other.m_useCount;
            addref();
            return *this;
        }

        GPUDeviceRef& operator=( GPUDeviceRef&& other )
        {
            if ( this == &other )
                return *this;

            if ( other.m_useCount == nullptr ) {
                deref();
                return *this;
            }

            m_device         = other.m_device;
            m_useCount       = other.m_useCount;
            other.m_device   = nullptr;
            other.m_useCount = nullptr;
            return *this;
        }

        ~GPUDeviceRef()
        {
            // could already be empty because we moved a nullptr into it
            if ( m_useCount == nullptr )
                return;

            if ( deref() == 1 /* was it the last reference? */ ) {
                delete m_useCount;
                m_device = nullptr;
            }
        }

        operator SDL_GPUDevice*() const
        {
            return m_device;
        }

        int use_count()
        {
            return m_useCount->load( std::memory_order_relaxed );
        }

    private:
        uint32_t addref()
        {
            return m_useCount->fetch_add( 1 );
        }

        uint32_t deref()
        {
            return m_useCount->fetch_sub( 1 );
        }

    private:
        SDL_GPUDevice*        m_device   = nullptr;
        std::atomic_uint32_t* m_useCount = nullptr;
    };
#else
    using GPUDeviceRef = SDL_GPUDevice*;
#endif

    class GPURenderer
    {
        GPURenderer() = default;

    public:
        ~GPURenderer();

        [[nodiscard]]
        static auto create() -> std::optional<Own<GPURenderer>>;
        Result      initialize( Window* window, AssetManager* assetmanager );

        Window*      get_window() const;
        GPUDeviceRef get_gpudevice() const;
        bool         has_window();
        bool         enable_vsync( bool enabled );

        void on_synchronize();
        void render();    // process all available rendercommands and send them to the gpu

        // Important: needs to be externally synchronized when adding rendercommands from multiple threads
        // currently the commands are only synchronized update and main thread once per frame
        void add_view_projection( const DXSM::Matrix view_projection );
        void add_sprite( const Sprite* sprite );
        void add_imgui_draw_data( ImDrawData* draw_data );

    private:
        void retrieve_shaderformatinfo();

    private:
        bool m_initialized = false;

        class PipelineProcessor;
        Own<PipelineProcessor> m_pipelineProcessor = nullptr;

        GPUDeviceRef m_sdlGPUDevice = nullptr;

        Window*      m_window = nullptr;
        bool         m_vsync  = true;
        // TODO: Create some kind of frameBuffer object
        DXSM::Matrix m_viewProjection;
    };
}    // namespace InnoEngine
