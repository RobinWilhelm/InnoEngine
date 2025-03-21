#include "InnoEngine/iepch.h"
#include "InnoEngine/DebugUI.h"

#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdl3.h"

#include "InnoEngine/Application.h"
#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/RenderCommandBuffer.h"

#include "InnoEngine/graphics/Camera.h"

namespace InnoEngine
{
    auto DebugUI::create( GPURenderer* renderer ) -> std::optional<Own<DebugUI>>
    {
        (void)renderer;
        IE_ASSERT( renderer != nullptr );
        Own<DebugUI> debugui = Own<DebugUI>( new DebugUI );
        // debugui->set_style();
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

        set_style();

        ImGui::SetNextWindowSize( { 300, 300 }, ImGuiCond_Always );
        if ( ImGui::Begin( "InnoEngine Debug Info", &m_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings ) ) {

            auto                   app = CoreAPI::get_application();
            const FrameTimingInfo& fti = app->get_frame_timings();

            const RenderStatistics& render_stats = renderer->get_statistics();

            if ( ImGui::BeginTabBar( "IEDebugMenuTabBar" ) ) {

                if ( ImGui::BeginTabItem( "Overview" ) ) {
                    ImGui::Text( "Driver: %s", renderer->get_devicedriver(), app->get_fps() );
                    ImGui::Text( "VSync: %s", renderer->vsync_enabled() ? "Enabled" : "Disabled" );

                    ImGui::Text( "FPS: %.0f", app->get_fps() );
                    ImGui::Text( "Frame Time: %.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );

                    if ( fti.FixedSimulationFrequency != 0 ) {
                        ImGui::Text( "Simulation frequency: %i", fti.FixedSimulationFrequency );
                    }
                    else {
                        ImGui::Text( "Simulation frequency: Unlimited" );
                    }

                    ImGui::NewLine();
                    ImGui::Text( "Pipeline commands: %u", render_stats.TotalCommands );
                    ImGui::Text( "Command buffer size : %.2f MB", static_cast<float>( render_stats.TotalBufferSize ) / 1024 / 1024 );
                    ImGui::Text( "SDL draw calls : %u", render_stats.TotalDrawCalls );
                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Frame Timings" ) ) {
                    if ( ImGui::BeginTable( "DebugTimingsTable", 2, ImGuiTabBarFlags_None ) ) {
                        ImGui::TableSetupColumn( "#Description", ImGuiTableColumnFlags_WidthFixed, 150 );
                        ImGui::TableSetupColumn( "#Timing", ImGuiTableColumnFlags_WidthFixed, 50 );
                        ImGui::TableNextColumn();

                        if ( app->running_mutithreaded() == false ) {
                            ImGui::Text( "Main Thread:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Renderer:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::ProcessRenderCommands ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Wait for GPU:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::GPUSwapChainWait ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Unindent();
                            ImGui::Text( "Layers:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", ( app->get_timing( ProfilePoint::LayerUpdate ) + app->get_timing( ProfilePoint::LayerRender ) ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Event:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::LayerEvent ) * 1000 );
                            ImGui::TableNextColumn();
                            ImGui::Text( "Update:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::LayerUpdate ) * 1000 );
                            ImGui::TableNextColumn();
                            ImGui::Text( "Render:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::LayerRender ) * 1000 );
                            ImGui::TableNextColumn();
                        }
                        else {
                            ImGui::Text( "Render Thread:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::MainThreadTotal ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Renderer:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::ProcessRenderCommands ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Wait for GPU:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::GPUSwapChainWait ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Unindent();
                            ImGui::Text( "Wait and Sync:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::WaitAndSynchronize ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Separator();

                            ImGui::Unindent();
                            ImGui::Text( "Layer Thread:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::UpdateThreadTotal ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Layers:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", ( app->get_timing( ProfilePoint::LayerUpdate ) + app->get_timing( ProfilePoint::LayerRender ) ) * 1000 );
                            ImGui::TableNextColumn();

                            ImGui::Indent();
                            ImGui::Text( "Event:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::LayerEvent ) * 1000 );
                            ImGui::TableNextColumn();
                            ImGui::Text( "Update:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::LayerUpdate ) * 1000 );
                            ImGui::TableNextColumn();
                            ImGui::Text( "Render:" );
                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2f ms", app->get_timing( ProfilePoint::LayerRender ) * 1000 );
                            ImGui::TableNextColumn();
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Camera" ) ) {
                    Camera*     camera  = CoreAPI::get_camera();
                    const auto& cam_pos = camera->get_position();

                    ImGui::Text( "Position: %.1f %.1f %.1f", cam_pos.x, cam_pos.y, cam_pos.z );
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        imgui_end_frame( renderer );
    }

    bool DebugUI::handle_event( const SDL_Event& event )
    {
        ImGui_ImplSDL3_ProcessEvent( &event );

        if ( event.type == SDL_EVENT_KEY_DOWN ) {
            if ( event.key.scancode == SDL_SCANCODE_GRAVE )
                m_open = !m_open;
        }

        switch ( event.type ) {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_WHEEL:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if ( ImGui::GetIO().WantCaptureMouse )
                return true;  
            break;
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if ( ImGui::GetIO().WantCaptureKeyboard )
                return true;
        }
        return false;
    }

    void DebugUI::set_style()
    {
        ImGuiStyle& style  = ImGui::GetStyle();
        ImVec4*     colors = style.Colors;

        // Base Colors
        ImVec4 bgColor           = ImVec4( 0.10f, 0.105f, 0.11f, 1.00f );
        ImVec4 lightBgColor      = ImVec4( 0.15f, 0.16f, 0.17f, 1.00f );
        ImVec4 panelColor        = ImVec4( 0.17f, 0.18f, 0.19f, 1.00f );
        ImVec4 panelHoverColor   = ImVec4( 0.20f, 0.22f, 0.24f, 1.00f );
        ImVec4 panelActiveColor  = ImVec4( 0.23f, 0.26f, 0.29f, 1.00f );
        ImVec4 textColor         = ImVec4( 0.86f, 0.87f, 0.88f, 1.00f );
        ImVec4 textDisabledColor = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
        ImVec4 borderColor       = ImVec4( 0.14f, 0.16f, 0.18f, 1.00f );

        // Text
        colors[ ImGuiCol_Text ]         = textColor;
        colors[ ImGuiCol_TextDisabled ] = textDisabledColor;

        // Windows
        colors[ ImGuiCol_WindowBg ]     = bgColor;
        colors[ ImGuiCol_ChildBg ]      = bgColor;
        colors[ ImGuiCol_PopupBg ]      = bgColor;
        colors[ ImGuiCol_Border ]       = borderColor;
        colors[ ImGuiCol_BorderShadow ] = borderColor;

        // Headers
        colors[ ImGuiCol_Header ]        = panelColor;
        colors[ ImGuiCol_HeaderHovered ] = panelHoverColor;
        colors[ ImGuiCol_HeaderActive ]  = panelActiveColor;

        // Buttons
        colors[ ImGuiCol_Button ]        = panelColor;
        colors[ ImGuiCol_ButtonHovered ] = panelHoverColor;
        colors[ ImGuiCol_ButtonActive ]  = panelActiveColor;

        // Frame BG
        colors[ ImGuiCol_FrameBg ]        = lightBgColor;
        colors[ ImGuiCol_FrameBgHovered ] = panelHoverColor;
        colors[ ImGuiCol_FrameBgActive ]  = panelActiveColor;

        // Tabs
        colors[ ImGuiCol_Tab ]                = panelColor;
        colors[ ImGuiCol_TabHovered ]         = panelHoverColor;
        colors[ ImGuiCol_TabActive ]          = panelActiveColor;
        colors[ ImGuiCol_TabUnfocused ]       = panelColor;
        colors[ ImGuiCol_TabUnfocusedActive ] = panelHoverColor;

        // Title
        colors[ ImGuiCol_TitleBg ]          = bgColor;
        colors[ ImGuiCol_TitleBgActive ]    = bgColor;
        colors[ ImGuiCol_TitleBgCollapsed ] = bgColor;

        // Scrollbar
        colors[ ImGuiCol_ScrollbarBg ]          = bgColor;
        colors[ ImGuiCol_ScrollbarGrab ]        = panelColor;
        colors[ ImGuiCol_ScrollbarGrabHovered ] = panelHoverColor;
        colors[ ImGuiCol_ScrollbarGrabActive ]  = panelActiveColor;

        // Checkmark
        colors[ ImGuiCol_CheckMark ] = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );

        // Slider
        colors[ ImGuiCol_SliderGrab ]       = panelHoverColor;
        colors[ ImGuiCol_SliderGrabActive ] = panelActiveColor;

        // Resize Grip
        colors[ ImGuiCol_ResizeGrip ]        = panelColor;
        colors[ ImGuiCol_ResizeGripHovered ] = panelHoverColor;
        colors[ ImGuiCol_ResizeGripActive ]  = panelActiveColor;

        // Separator
        colors[ ImGuiCol_Separator ]        = borderColor;
        colors[ ImGuiCol_SeparatorHovered ] = panelHoverColor;
        colors[ ImGuiCol_SeparatorActive ]  = panelActiveColor;

        // Plot
        colors[ ImGuiCol_PlotLines ]            = textColor;
        colors[ ImGuiCol_PlotLinesHovered ]     = panelActiveColor;
        colors[ ImGuiCol_PlotHistogram ]        = textColor;
        colors[ ImGuiCol_PlotHistogramHovered ] = panelActiveColor;

        // Text Selected BG
        colors[ ImGuiCol_TextSelectedBg ] = panelActiveColor;

        // Modal Window Dim Bg
        colors[ ImGuiCol_ModalWindowDimBg ] = ImVec4( 0.10f, 0.105f, 0.11f, 0.5f );

        // Tables
        colors[ ImGuiCol_TableHeaderBg ]     = panelColor;
        colors[ ImGuiCol_TableBorderStrong ] = borderColor;
        colors[ ImGuiCol_TableBorderLight ]  = borderColor;
        colors[ ImGuiCol_TableRowBg ]        = bgColor;
        colors[ ImGuiCol_TableRowBgAlt ]     = lightBgColor;

        // Styles
        style.FrameBorderSize   = 1.0f;
        style.FrameRounding     = 2.0f;
        style.WindowBorderSize  = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.ScrollbarSize     = 12.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabMinSize       = 7.0f;
        style.GrabRounding      = 2.0f;
        style.TabBorderSize     = 1.0f;
        style.TabRounding       = 2.0f;

        // Reduced Padding and Spacing
        style.WindowPadding    = ImVec2( 5.0f, 5.0f );
        style.FramePadding     = ImVec2( 4.0f, 3.0f );
        style.ItemSpacing      = ImVec2( 6.0f, 4.0f );
        style.ItemInnerSpacing = ImVec2( 4.0f, 4.0f );

        /*
        // Font Scaling
        ImGuiIO& io        = ImGui::GetIO();
        io.FontGlobalScale = 1.00f;

        io.Fonts->AddFontDefault();

        float baseFontSize = 18.0f;
        float iconFontSize = baseFontSize * 2.0f / 3.0f;


        // merge in icons from Font Awesome
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        io.Fonts->AddFontFromFileTTF(
        (std::string(RESOURCE_DIR) + "/fonts/" + FONT_ICON_FILE_NAME_FA).c_str(), iconFontSize,
        &icons_config, icons_ranges);
        */
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
