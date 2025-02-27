

set(Sandbox_Sources 
	"src/main.cpp"
	
	"src/Sandbox.h"
	"src/Sandbox.cpp"
	"src/DemoScene.h"
	"src/DemoScene.cpp"

)

PREPEND(Sandbox_Sources)

set(Sandbox_Sources ${Sandbox_Sources}  PARENT_SCOPE)