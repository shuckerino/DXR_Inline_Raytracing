include("../../CreateApp.cmake")
set(SOURCES "./src/ConstantBufferApp.cpp")
set(SHADERS "./shaders/ConstantBuffer.hlsl")
create_app(T10ConstantBuffer "${SOURCES}" "${SHADERS}")

