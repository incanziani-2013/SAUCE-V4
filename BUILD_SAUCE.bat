@echo off
setlocal
cd /d "%~dp0"
echo ==========================================
echo       SAUCE V1 PRO - WINDOWS VST3
echo ==========================================
where cmake >nul 2>nul || (echo ERROR: Install CMake 3.22+ & pause & exit /b 1)
where git >nul 2>nul || (echo ERROR: Install Git for Windows & pause & exit /b 1)
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (echo CMake configure failed. & pause & exit /b 1)
cmake --build build --config Release --target SAUCE_VST3
if errorlevel 1 (echo Compilation failed. Scroll up and send me the error. & pause & exit /b 1)
echo.
echo Build succeeded. Look for:
echo build\SAUCE_artefacts\Release\VST3\SAUCE.vst3
echo If not found, search inside the build folder for SAUCE.vst3.
echo Copy the complete .vst3 bundle to:
echo C:\Program Files\Common Files\VST3\
pause
