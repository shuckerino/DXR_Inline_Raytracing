include("../../CreateApp.cmake")
set(SOURCES "./src/TextureApp.cpp")
set(SHADERS "./shaders/Texture.hlsl")
create_app(T09Texture "${SOURCES}" "${SHADERS}")


