@echo off

SET buildDir=%~dp0%build

if not exist %buildDir% (
	@echo on
	echo Creating directory build
	@echo.
	mkdir %buildDir%
)

@echo Compiling shaders
@echo.

@echo off
if not -%1-==-- glsl-compiler.exe %1
if -%1-==-- for %%f in (*.glsl) do glsl-compiler.exe %%~nf	

@echo on

@echo.
@echo Finished compiling shaders
@echo.


@echo Creating shaderPipelineInput.h
@echo off
spirv-reflection.exe build
@echo on
@echo Finish
