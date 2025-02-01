include("../../CreateApp.cmake")
set(SOURCES "./src/RootConstantApp.cpp")
set(SHADERS "./shaders/RootConstant.hlsl")
create_app(T05RootConstant "${SOURCES}" "${SHADERS}")

