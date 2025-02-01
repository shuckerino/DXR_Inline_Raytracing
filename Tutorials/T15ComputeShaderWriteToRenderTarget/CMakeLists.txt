include("../../CreateApp.cmake")
set(SOURCES "./src/ComputeShaderWriteToRenderTarget.cpp")
set(SHADERS "./shaders/WriteToRenderTarget.hlsl")
create_app(T15ComputeShaderWriteToRenderTarget "${SOURCES}" "${SHADERS}")
