#include "imgui.h"

#include "BaseTypes.h"
#include "Layer.h"

#include <optional>
#include <memory>

namespace InnoEngine
{
    class GPURenderer;
    class ImGuiPipeline;
    class Application;

    class DebugUI : public Layer
    {
        DebugUI( Application* parent ) :
            Layer( parent ) { };

    public:
        [[nodiscard]]
        static auto create( GPURenderer* renderer ) -> std::optional<Own<DebugUI>>;

        // Inherited via Layer
        void update( double deltaTime ) override;
        void render( float interpFactor, GPURenderer* pRenderer ) override;
        bool handle_event( const SDL_Event& pEvent ) override;

    private:
        void set_style();

        void imgui_begin_frame();
        void imgui_end_frame( GPURenderer* pRenderer );

    private:
        bool m_open = false;
    };
}    // namespace InnoEngine
