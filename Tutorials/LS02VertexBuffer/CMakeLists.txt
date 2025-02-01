include("../../CreateApp.cmake")
set(SOURCES "./src/VertexBuffer.cpp")
set(SHADERS "./shaders/VertexBuffer.hlsl")
create_app(LS02VertexBuffer "${SOURCES}" "${SHADERS}")

