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
            uint16_t ViewMatrixIndex   = 0;
            uint16_t RenderTargetIndex = 0;
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
            float         pad[ 2 ];
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
            float         pad;
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
            float         pad[ 2 ];
        };

        using QuadCommandList   = std::vector<QuadCommand>;
        using LineCommandList   = std::vector<LineCommand>;
        using CircleCommandList = std::vector<CircleCommand>;

        const uint32_t QuadBatchSize   = 20000;
        const uint32_t LineBatchSize   = 20000;
        const uint32_t CircleBatchSize = 20000;

        ~Primitive2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* asset_manager );
        void     prepare_render( const QuadCommandList&   quad_command_list,
                                 const LineCommandList&   line_command_list,
                                 const CircleCommandList& circle_command_list );
        uint32_t swapchain_render( const std::vector<DXSM::Matrix>& view_projections_list,
                                   SDL_GPUCommandBuffer*            gpu_cmd_buf,
                                   SDL_GPURenderPass*               render_pass );

        void sort_quad_commands( const QuadCommandList& quad_command_list );
        void sort_line_commands( const LineCommandList& quad_command_list );
        void sort_circle_commands( const CircleCommandList& circle_command_list );

    private:
        Result load_quad_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo );
        Result load_line_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo );
        Result load_circle_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo );

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
