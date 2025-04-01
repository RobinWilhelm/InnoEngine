#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"
#include "InnoEngine/graphics/GPUBatchBuffer.h"

#include "InnoEngine/graphics/Viewport.h"
#include "InnoEngine/graphics/RenderContext.h"

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
            RenderCommandBufferIndexType ContextIndex = InvalidRenderCommandBufferIndex;
        };

        struct QuadCommand : RenderCommandBase
        {
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Color   Color;
            float         Rotation;
        };

        struct QuadStorageBufferLayout
        {
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Color   Color;
            float         Rotation;
            float         Depth;
            uint32_t      ContextIndex;
            float         pad[ 1 ];
        };

        struct LineCommand : RenderCommandBase
        {
            DXSM::Vector2 Start;
            DXSM::Vector2 End;
            DXSM::Color   Color;
            float         Thickness;
            float         EdgeFade;
        };

        struct LineStorageBufferLayout
        {
            DXSM::Vector2 Start;
            DXSM::Vector2 End;
            DXSM::Color   Color;
            float         Thickness;
            float         EdgeFade;
            float         Depth;
            uint32_t      ContextIndex;
        };

        struct CircleCommand : RenderCommandBase
        {
            DXSM::Color   Color;
            DXSM::Vector2 Position;
            float         Radius;
            float         Thickness;
            float         Fade;
        };

        struct CircleStorageBufferLayout
        {
            DXSM::Color   Color;
            DXSM::Vector2 Position;
            float         Radius;
            float         Thickness;
            float         Fade;
            float         Depth;
            uint32_t      ContextIndex;
            float         pad[ 1 ];
        };

        using QuadCommandList   = std::vector<QuadCommand>;
        using LineCommandList   = std::vector<LineCommand>;
        using CircleCommandList = std::vector<CircleCommand>;

        const uint32_t QuadBatchSize   = 20000;
        const uint32_t LineBatchSize   = 20000;
        const uint32_t CircleBatchSize = 20000;

        ~Primitive2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* asset_manager );
        uint32_t prepare_render_opaque( const QuadCommandList&   quad_command_list,
                                        const LineCommandList&   line_command_list,
                                        const CircleCommandList& circle_command_list );
        uint32_t prepare_render( const QuadCommandList&   quad_command_list,
                                 const LineCommandList&   line_command_list,
                                 const CircleCommandList& circle_command_list );
        uint32_t swapchain_render( const RenderContextFrameData& render_ctx_data,
                                   SDL_GPURenderPass*            render_pass );

        void sort_quad_commands( const QuadCommandList& quad_command_list, bool opaque );
        void sort_line_commands( const LineCommandList& quad_command_list, bool opaque );
        void sort_circle_commands( const CircleCommandList& circle_command_list, bool opaque );

    private:
        Result load_quad_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo );
        Result load_line_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo );
        Result load_circle_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo );

        uint32_t prepare_batches();

    private:
        bool         m_Initialized = false;
        GPUDeviceRef m_Device      = nullptr;

        SDL_GPUGraphicsPipeline*                                       m_QuadPipeline = nullptr;
        std::vector<const QuadCommand*>                                m_SortedQuadCommands;    // objects owned by the RenderCommandBuffer
        Ref<GPUBatchStorageBuffer<QuadStorageBufferLayout, BatchData>> m_QuadGPUBatch = nullptr;

        SDL_GPUGraphicsPipeline*                                       m_LinePipeline = nullptr;
        std::vector<const LineCommand*>                                m_SortedLineCommands;    // objects owned by the RenderCommandBuffer
        Ref<GPUBatchStorageBuffer<LineStorageBufferLayout, BatchData>> m_LineGPUBatch = nullptr;

        SDL_GPUGraphicsPipeline*                                         m_CirclePipeline = nullptr;
        std::vector<const CircleCommand*>                                m_SortedCircleCommands;    // objects owned by the RenderCommandBuffer
        Ref<GPUBatchStorageBuffer<CircleStorageBufferLayout, BatchData>> m_CircleGPUBatch = nullptr;
    };

    using QuadCommandBuffer   = Primitive2DPipeline::QuadCommandList;
    using LineCommandBuffer   = Primitive2DPipeline::LineCommandList;
    using CircleCommandBuffer = Primitive2DPipeline::CircleCommandList;
}    // namespace InnoEngine
