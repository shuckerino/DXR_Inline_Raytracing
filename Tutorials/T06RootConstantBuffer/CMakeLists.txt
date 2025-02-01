include("../../CreateApp.cmake")
set(SOURCES "./src/RootConstantBufferApp.cpp")
set(SHADERS "./shaders/RootConstantBuffer.hlsl")
create_app(T06RootConstantBuffer "${SOURCES}" "${SHADERS}")

