#include "SDL3/SDL_gpu.h"
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

namespace InnoEngine
{
    class GPURenderer;
    class AssetManager;
    class Shader;
    template <typename T>
    class AssetRepository;

    class Primitive2DPipeline
    {
    public:
        struct BatchData
        {
            FrameBufferIndex TextureIndex = -1;
            uint32_t         BufferIndex  = 0;
            uint32_t         CommandCount = 0;
        };

        struct QuadCommand
        {
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Color   Color;
        };

        struct QuadVertexLayout
        {
        };

        const uint32_t QuadBatchSize = 50000;
        using QuadCommandList        = std::vector<QuadCommand>;

        ~Primitive2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* asset_manager );
        void     prepare_render( const QuadCommandList& point_command_list );
        uint32_t swapchain_render( const DXSM::Matrix&    view_projection,
                                   const QuadCommandList& point_command_list,
                                   SDL_GPUCommandBuffer*  gpu_cmd_buf,
                                   SDL_GPURenderPass*     render_pass );

    private:
        Result load_quad_pipeline( SDL_Window* window, AssetRepository<Shader>* shader_repo );

    private:
        bool                     m_Initialized  = false;
        SDL_GPUGraphicsPipeline* m_QuadPipeline = nullptr;
        GPUDeviceRef             m_Device       = nullptr;

        SDL_GPUTransferBuffer* m_QuadTransferBuffer = nullptr;
    };
}    // namespace InnoEngine
