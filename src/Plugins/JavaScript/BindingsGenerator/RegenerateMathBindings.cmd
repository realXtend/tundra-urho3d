setlocal EnableDelayedExpansion

IF "%1"=="" (
    echo "Usage: RegenerateMathBindings <path to MathGeoLib checkout>"
    GOTO :ERROR
)

set MGL_DIR=%1
set ORIGINAL_DIR=%CD%

copy %CD%\MathBindings.doxyfile %MGL_DIR%
cd %MGL_DIR%
doxygen MathBindings.doxyfile
cd %ORIGINAL_DIR%
bin\release\BindingsGenerator.exe %MGL_DIR%\MathDocs\xml ../MathBindings %MGL_DIR%\src float2 float3 float4 float3x3 float3x4 float4x4 AABB Capsule Circle Frustum ^
    LCG Line LineSegment OBB Plane Quat Ray Transform Triangle Sphere Color

:ERROR