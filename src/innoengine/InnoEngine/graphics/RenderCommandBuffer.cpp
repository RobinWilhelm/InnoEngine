#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/RenderCommandBuffer.h"

namespace InnoEngine
{
    RenderCommandBuffer::RenderCommandBuffer()
    {
        RenderContextCommands.resize( 256 );
        for ( auto& render_ctx_cmd : RenderContextCommands ) {
            render_ctx_cmd.FontRegister    = &FontRegister;
            render_ctx_cmd.StringBuffer    = &StringBuffer;
            render_ctx_cmd.TextureRegister = &TextureRegister;
        }
    }

    /*
    RenderCommandBuffer& RenderCommandBuffer::operator=( RenderCommandBuffer& other )
    {
        if ( this == &other )
            return *this;

        Clear      = other.Clear;
        ClearColor = other.ClearColor;


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
    */

    void RenderCommandBuffer::clear()
    {
        Clear      = false;
        ClearColor = DXSM::Color( 0.0f, 0.0f, 0.0f, 0.0f );

        RenderContextList.clear();

        // clear the commands but not the rendercontexts itself
        for ( auto& render_ctx_cmds : RenderContextCommands ) {
            render_ctx_cmds.CircleRenderCommands.clear();
            render_ctx_cmds.SpriteRenderCommands.clear();
            render_ctx_cmds.QuadRenderCommands.clear();
            render_ctx_cmds.LineRenderCommands.clear();
            render_ctx_cmds.FontRenderCommands.clear();
        }
        ImGuiCommandBuffer.RenderCommandLists.clear();

        StringBuffer.clear();

        FontRegister.clear();
        TextureRegister.clear();
    }
}    // namespace InnoEngine
