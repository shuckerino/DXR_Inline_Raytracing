include("../../CreateApp.cmake")
set(SOURCES "./src/ComputeShaderPostProcess.cpp")
set(SHADERS "./shaders/ComputeShaderPostProcess.hlsl" "./shaders/Triangle.hlsl")
create_app(T16ComputeShaderPostProcess "${SOURCES}" "${SHADERS}")

