include("../../CreateApp.cmake")
set(SOURCES "./src/ComputeShaderVectorAdd.cpp")
set(SHADERS "./shaders/VectorAdd.hlsl")
create_app(T14ComputeShaderVectorAdd "${SOURCES}" "${SHADERS}")
