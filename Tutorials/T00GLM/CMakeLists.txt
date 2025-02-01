
include("../../CreateApp.cmake")
set(SOURCES "./src/GLMExample.cpp")
set(SHADERS "")
create_app(T00GLM "${SOURCES}" "${SHADERS}")
