include("../../CreateApp.cmake")
set(SOURCES "./src/VertexBufferApp.cpp")
set(SHADERS "./shaders/VertexBuffer.hlsl")
create_app(T07VertexBuffer "${SOURCES}" "${SHADERS}")
