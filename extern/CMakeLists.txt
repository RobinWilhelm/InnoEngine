set(DIRECTXTK_SOURCES 	
	"directxtk/SimpleMath.h"
	"directxtk/SimpleMath.cpp"
	"directxtk/SimpleMath.inl" 
)

set(IMGUI_SOURCES 
	"imgui/imgui.h"
	"imgui/imgui.cpp"
	"imgui/imconfig.h"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_impl_sdl3.cpp"
	"imgui/imgui_impl_sdl3.h"
	"imgui/imgui_impl_sdlgpu3.cpp"
	"imgui/imgui_impl_sdlgpu3.h"
	"imgui/imgui_internal.h"
	"imgui/imgui_impl_sdlgpu3_shaders.h"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imstb_rectpack.h"
	"imgui/imstb_textedit.h"
	"imgui/imstb_truetype.h"
)


set(EXTERNAL_SOURCES ${DIRECTXTK_SOURCES} ${IMGUI_SOURCES})
PREPEND(EXTERNAL_SOURCES)
set(EXTERNAL_SOURCES ${EXTERNAL_SOURCES}  PARENT_SCOPE)


set(IMGUI_INCLUDE_DIRS "imgui")
PREPEND(IMGUI_INCLUDE_DIRS)
set(IMGUI_INCLUDE_DIRS ${IMGUI_INCLUDE_DIRS}  PARENT_SCOPE)

set(DIRECTXTK_INCLUDE_DIRS "directxtk")
PREPEND(DIRECTXTK_INCLUDE_DIRS)
set(DIRECTXTK_INCLUDE_DIRS ${DIRECTXTK_INCLUDE_DIRS}  PARENT_SCOPE)

set(SDL3_INCLUDE_DIRS "SDL3/include")
PREPEND(SDL3_INCLUDE_DIRS)
set(SDL3_INCLUDE_DIRS ${SDL3_INCLUDE_DIRS}  PARENT_SCOPE)

set(SDL3_BUILD "SDL3/build")
PREPEND(SDL3_BUILD)
set(SDL3_BUILD ${SDL3_BUILD}  PARENT_SCOPE)

set(SDL3_IMAGE_INCLUDE_DIRS "SDL3_image/include")
PREPEND(SDL3_IMAGE_INCLUDE_DIRS)
set(SDL3_IMAGE_INCLUDE_DIRS ${SDL3_IMAGE_INCLUDE_DIRS}  PARENT_SCOPE)

set(SDL3_IMAGE_BUILD "SDL3_image/build")
PREPEND(SDL3_IMAGE_BUILD)
set(SDL3_IMAGE_BUILD ${SDL3_IMAGE_BUILD}  PARENT_SCOPE)

set(SDL3_SHADERCROSS_INCLUDE_DIRS "SDL3_shadercross/include")
PREPEND(SDL3_SHADERCROSS_INCLUDE_DIRS)
set(SDL3_SHADERCROSS_INCLUDE_DIRS ${SDL3_SHADERCROSS_INCLUDE_DIRS}  PARENT_SCOPE)

set(SDL3_SHADERCROSS_BUILD_DIRS "SDL3_shadercross/build")
PREPEND(SDL3_SHADERCROSS_BUILD_DIRS)
set(SDL3_SHADERCROSS_BUILD_DIRS ${SDL3_SHADERCROSS_BUILD_DIRS}  PARENT_SCOPE)




