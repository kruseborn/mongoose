@echo Compiling shaders...
CALL shaders.bat %1

@echo Copying shaderPipelineInput...
@echo off
xcopy shaderPipelineInput.h ..\..\src\engine\vulkan\ /d /y
@echo on