include("../../CreateApp.cmake")
set(SOURCES "./src/VertexIndexBufferApp.cpp")
set(SHADERS "./shaders/VertexIndexBuffer.hlsl")
create_app(T08VertexIndexBuffer "${SOURCES}" "${SHADERS}")
