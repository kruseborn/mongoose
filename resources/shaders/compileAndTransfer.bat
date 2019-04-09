@echo Compiling shaders...
CALL shaders.bat %1

@echo.
@echo Copying vtk shaders...

@echo off
xcopy build ..\..\build\shaders\ /d /y
@echo on


@echo Copying shaderPipelineInput...
@echo off
xcopy shaderPipelineInput.h ..\..\src\engine\vulkan\ /d /y
@echo on