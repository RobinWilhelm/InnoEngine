#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/RenderCommandBuffer.h"

namespace InnoEngine
{
    RenderCommandBuffer::RenderCommandBuffer()
    {
        SpriteRenderCommands.reserve( 20000 );
        QuadRenderCommands.reserve( 20000 );
        LineRenderCommands.reserve( 20000 );
        CircleRenderCommands.reserve( 20000 );
        FontRenderCommands.reserve( 20000 );
    }

    RenderCommandBuffer& RenderCommandBuffer::operator=( RenderCommandBuffer& other )
    {
        if ( this == &other )
            return *this;

        Clear        = other.Clear;
        ClearColor   = other.ClearColor;
        CameraMatrix = other.CameraMatrix;

        SpriteRenderCommands.resize( other.SpriteRenderCommands.size() );
        std::memcpy( static_cast<void*>( SpriteRenderCommands.data() ),
                     static_cast<void*>( other.SpriteRenderCommands.data() ),
                     other.SpriteRenderCommands.size() * sizeof( Sprite2DPipeline::Command ) );

        QuadRenderCommands.resize( other.QuadRenderCommands.size() );
        std::memcpy( static_cast<void*>( QuadRenderCommands.data() ),
                     static_cast<void*>( other.QuadRenderCommands.data() ),
                     other.QuadRenderCommands.size() * sizeof( Primitive2DPipeline::QuadCommand ) );

        LineRenderCommands.resize( other.LineRenderCommands.size() );
        std::memcpy( static_cast<void*>( LineRenderCommands.data() ),
                     static_cast<void*>( other.LineRenderCommands.data() ),
                     other.LineRenderCommands.size() * sizeof( Primitive2DPipeline::LineCommand ) );

        CircleRenderCommands.resize( other.CircleRenderCommands.size() );
        std::memcpy( static_cast<void*>( CircleRenderCommands.data() ),
                     static_cast<void*>( other.CircleRenderCommands.data() ),
                     other.CircleRenderCommands.size() * sizeof( Primitive2DPipeline::CircleCommand ) );

        ImGuiCommandBuffer = other.ImGuiCommandBuffer;

        FontRenderCommands.resize( other.FontRenderCommands.size() );
        std::memcpy( static_cast<void*>( FontRenderCommands.data() ),
                     static_cast<void*>( other.FontRenderCommands.data() ),
                     other.FontRenderCommands.size() * sizeof( Font2DPipeline::Command ) );

        StringBuffer = other.StringBuffer;

        FontRegister    = other.FontRegister;
        TextureRegister = other.TextureRegister;

        return *this;
    }

    void RenderCommandBuffer::clear()
    {
        LastFrameStats = Stats();

        LastFrameStats.TotalBufferSize += sizeof( DXSM::Color );
        LastFrameStats.TotalBufferSize += sizeof( DXSM::Matrix );
        LastFrameStats.TotalBufferSize += TextureRegister.size() * sizeof( Ref<Texture2D> );

        LastFrameStats.TotalCommands += SpriteRenderCommands.size();
        LastFrameStats.TotalBufferSize += SpriteRenderCommands.size() * sizeof( Sprite2DPipeline::Command );

        LastFrameStats.TotalCommands += QuadRenderCommands.size();
        LastFrameStats.TotalBufferSize += QuadRenderCommands.size() * sizeof( Primitive2DPipeline::QuadCommand );

        LastFrameStats.TotalCommands += LineRenderCommands.size();
        LastFrameStats.TotalBufferSize += LineRenderCommands.size() * sizeof( Primitive2DPipeline::LineCommand );

        LastFrameStats.TotalCommands += CircleRenderCommands.size();
        LastFrameStats.TotalBufferSize += CircleRenderCommands.size() * sizeof( Primitive2DPipeline::CircleCommand );

        LastFrameStats.TotalBufferSize += FontRegister.size() * sizeof( Ref<Font> );
        LastFrameStats.TotalCommands += FontRenderCommands.size();
        LastFrameStats.TotalBufferSize += FontRenderCommands.size() * sizeof( Font2DPipeline::Command );
        LastFrameStats.TotalBufferSize += StringBuffer.size();

        for ( const auto& rcmd : ImGuiCommandBuffer.RenderCommandLists ) {
            LastFrameStats.TotalCommands += rcmd.CommandBuffer.size();
            LastFrameStats.TotalBufferSize += rcmd.CommandBuffer.size() * sizeof( ImDrawCmd );
            LastFrameStats.TotalBufferSize += rcmd.IndexBuffer.size() * sizeof( ImDrawIdx );
            LastFrameStats.TotalBufferSize += rcmd.VertexBuffer.size() * sizeof( ImDrawVert );
        }

        LastFrameStats.TotalDrawCalls = SpriteDrawCalls + FontDrawCalls + ImGuiDrawCalls;

        Clear      = false;
        ClearColor = DXSM::Color( 0.0f, 0.0f, 0.0f, 0.0f );
        CameraMatrix = DXSM::Matrix::Identity;
        SpriteRenderCommands.clear();
        QuadRenderCommands.clear();
        LineRenderCommands.clear();
        CircleRenderCommands.clear();
        ImGuiCommandBuffer.RenderCommandLists.clear();

        FontRenderCommands.clear();
        StringBuffer.clear();

        FontRegister.clear();
        TextureRegister.clear();

        SpriteDrawCalls = 0;
        FontDrawCalls   = 0;
        ImGuiDrawCalls  = 0;
    }

    const RenderCommandBuffer::Stats& RenderCommandBuffer::get_stats() const
    {
        return LastFrameStats;
    }

}    // namespace InnoEngine
