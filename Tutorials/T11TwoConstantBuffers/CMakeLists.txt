include("../../CreateApp.cmake")
set(SOURCES "./src/TwoConstantBuffersApp.cpp")
set(SHADERS "./shaders/TwoConstantBuffers.hlsl")
create_app(T11TwoConstantBuffers "${SOURCES}" "${SHADERS}")

