#include "InnoEngine/iepch.h"
#include "InnoEngine/DebugUI.h"

#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdl3.h"

#include "InnoEngine/Application.h"
#include "InnoEngine/Renderer.h"
#include "InnoEngine/RenderCommandBuffer.h"

namespace InnoEngine
{
    auto DebugUI::create( GPURenderer* renderer ) -> std::optional<Own<DebugUI>>
    {
        (void)renderer;
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

            ImGui::Text( "Driver: %s FPS: %.0f", renderer->get_devicedriver(), app->get_fps() );
            ImGui::Text( "VSync: %s", renderer->vsync_enabled() ? "Enabled" : "Disabled" );
            ImGui::Spacing();

            FrameTimingInfo fti = app->get_frame_timings();
            if ( fti.FixedSimulationFrequency != 0 ) {
                ImGui::Text( "Simulation frequency: %i", fti.FixedSimulationFrequency );
                ImGui::Text( "Frame Time: %.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );
            }
            else {
                ImGui::Text( "Simulation frequency: Unlimited" );
                ImGui::Text( "Frame Time: %.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );
            }
            ImGui::Spacing();

            ImGui::Text( "Main Thread Total: %.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );

            ImGui::Text( "\tRenderer: %.2f ms", app->get_timing( ProfilePoint::ProcessRenderCommands ) * 1000 );
            if ( app->running_mutithreaded() == false ) {

                ImGui::Text( "\tLayer Update: %.2f ms", app->get_timing( ProfilePoint::LayerUpdate ) * 1000 );
                ImGui::Text( "\tLayer Render: %.2f ms", app->get_timing( ProfilePoint::LayerRender ) * 1000 );
            }
            else {
                ImGui::Text( "\tWait and Sync : %.2f ms", app->get_timing( ProfilePoint::WaitAndSynchronize ) * 1000 );

                ImGui::Spacing();
                ImGui::Text( "Update Thread Total: %.2f ms", app->get_timing( ProfilePoint::UpdateThreadTotal ) * 1000 );
                ImGui::Text( "\tLayer Update: %.2f ms", app->get_timing( ProfilePoint::LayerUpdate ) * 1000 );
                ImGui::Text( "\tLayer Render: %.2f ms", app->get_timing( ProfilePoint::LayerRender ) * 1000 );
            }

            ImGui::Spacing();
            ImGui::Text("Total rendercommands : %u", renderer->get_render_command_buffer()->get_stats().TotalCommands);
            ImGui::Text("Command buffer size : %.2f MB", static_cast<float>(renderer->get_render_command_buffer()->get_stats().TotalBufferSize) / 1024 / 1024);

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
