#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"
#include "InnoEngine/graphics/GPUBatchBuffer.h"

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
            uint32_t Dummy;
        };

        struct QuadCommand
        {
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Color   Color;
            float         Depth;
            float         Rotation;
        };

        struct QuadStorageBufferLayout
        {
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Color   Color;
            float         Depth;
            float         Rotation;
            float         pad[ 2 ];
        };

        struct LineCommand
        { };

        struct CircleCommand
        { };

        using QuadCommandList        = std::vector<QuadCommand>;
        using LineCommandList        = std::vector<LineCommand>;
        using CircleCommandList      = std::vector<CircleCommand>;
        const uint32_t QuadBatchSize = 20000;

        ~Primitive2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* asset_manager );
        void     prepare_render( const QuadCommandList& quad_command_list );
        uint32_t swapchain_render( const DXSM::Matrix&   view_projection,
                                   SDL_GPUCommandBuffer* gpu_cmd_buf,
                                   SDL_GPURenderPass*    render_pass );

        void sort_quad_commands( const QuadCommandList& quad_command_list );

    private:
        Result load_quad_pipeline( SDL_Window* window, AssetRepository<Shader>* shader_repo );

    private:
        bool         m_Initialized = false;
        GPUDeviceRef m_Device      = nullptr;

        SDL_GPUGraphicsPipeline*                                       m_QuadPipeline = nullptr;
        std::vector<const QuadCommand*>                                m_SortedQuadCommands;    // objects owned by the RenderCommandBuffer
        Ref<GPUBatchStorageBuffer<QuadStorageBufferLayout, BatchData>> m_QuadGPUBatch = nullptr;
    };

    using QuadCommandBuffer   = Primitive2DPipeline::QuadCommandList;
    using LineCommandBuffer   = Primitive2DPipeline::LineCommandList;
    using CircleCommandBuffer = Primitive2DPipeline::CircleCommandList;
}    // namespace InnoEngine
