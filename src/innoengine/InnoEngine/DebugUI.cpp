#include "iepch.h"
#include "DebugUI.h"

#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdl3.h"

#include "Application.h"
#include "Renderer.h"

namespace InnoEngine
{
    auto DebugUI::create( GPURenderer* renderer ) -> std::optional<Own<DebugUI>>
    {
        IE_ASSERT( renderer != nullptr );
        Own<DebugUI> debugui = Own<DebugUI>( new DebugUI );
        return debugui;
    }

    void DebugUI::update( double delta_time )
    {
        (void)delta_time;
    }

    void DebugUI::render( float interp_factor, GPURenderer* renderer )
    {
        (void)interp_factor;
        imgui_begin_frame();

        if ( ImGui::Begin( "Debug" ) ) {

            auto app = CoreAPI::get_application();

            ImGui::Text( "Driver: %s", renderer->get_devicedriver());
            ImGui::Text( "VSync: %s", renderer->vsync_enabled() ? "Enabled" : "Disabled" );
            ImGui::Text( "FPS: %.0f", app->get_fps() );
            ImGui::Text( "Frametime: %.2f ms", app->get_timing( ProfilePoint::TotalFrame ) * 1000 );

            ImGui::Text( "Mainthread: %.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );
            ImGui::Text( "Updatethread: %.2f ms", app->get_timing( ProfilePoint::UpdateThreadTotal ) * 1000 );

            ImGui::Text( "Update: %.2f ms", app->get_timing( ProfilePoint::Update ) * 1000 );
            ImGui::Text( "Render: %.2f ms", app->get_timing( ProfilePoint::Render ) * 1000 );
        }
        ImGui::End();
        imgui_end_frame( renderer );
    }

    bool DebugUI::handle_event( const SDL_Event& event )
    {
        return ImGui_ImplSDL3_ProcessEvent( &event );
    }

    void DebugUI::imgui_begin_frame()
    {
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void DebugUI::imgui_end_frame( GPURenderer* renderer )
    {
        (void)renderer;
        ImGui::Render();
        renderer->add_imgui_draw_data( ImGui::GetDrawData() );
    }
}    // namespace InnoEngine
