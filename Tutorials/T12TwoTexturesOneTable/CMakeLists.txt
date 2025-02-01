include("../../CreateApp.cmake")
set(SOURCES "./src/TwoTexturesOneTableApp.cpp")
set(SHADERS "./shaders/TwoTextures.hlsl")
create_app(T12TwoTexturesOneTable "${SOURCES}" "${SHADERS}")

