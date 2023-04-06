@ECHO OFF

REM COLOR HELPERS
SET _bBlack=[40m
Set _fRed=[31m
SET _fGreen=[92m
SET _ResetColor=[0m

ECHO.
ECHO %_fGreen%%_bBlack%Cleaning cache.%_ResetColor%
IF EXIST "%CD%\Binaries" ( RMDIR /S /Q "%CD%\Binaries" )
IF EXIST "%CD%\Intermediate" ( RMDIR /S /Q "%CD%\Intermediate" )

ECHO.
ECHO %_fGreen%%_bBlack%Copying Implot to ImGui.%_ResetColor%
COPY /Y ImPlot\*.h ImGui\
COPY /Y ImPlot\*.cpp ImGui\

ECHO.
ECHO %_fGreen%%_bBlack%CMake Intermediate\ImGui .%_ResetColor%
cmake . -B Intermediate
IF %ERRORLEVEL% NEQ 0 ( GOTO :ERROR )

ECHO.
ECHO %_fGreen%%_bBlack%CMake ImGui Debug.%_ResetColor%
cmake --build Intermediate --config Debug
IF %ERRORLEVEL% NEQ 0 ( GOTO :ERROR )

ECHO.
ECHO %_fGreen%%_bBlack%Renaming Debug\ImGui.lib to Debug\ImGui_Debug.lib .%_ResetColor%
COPY /Y Binaries\ImGui.lib Binaries\ImGui_Debug.lib
COPY /Y Intermediate\CMakeFiles\ImGui.dir\ImGui.pdb Binaries\ImGui_Debug.pdb
IF %ERRORLEVEL% NEQ 0 ( GOTO :ERROR )

ECHO.
ECHO %_fGreen%%_bBlack%CMake ImGui Release.%_ResetColor%
cmake --build Intermediate --config Release
IF %ERRORLEVEL% NEQ 0 ( GOTO :ERROR )

ECHO.
ECHO %_fGreen%%_bBlack%Copying libs to DFoundryFX Plugin Binaries Folder.%_ResetColor%
IF NOT EXIST "..\..\..\Binaries\Win64\" ( MKDIR "..\..\..\Binaries\Win64" )
COPY /Y Binaries\ImGui_Debug*.* ..\..\..\Binaries\Win64\
COPY /Y Binaries\ImGui*.* ..\..\..\Binaries\Win64\
IF %ERRORLEVEL% NEQ 0 ( GOTO :ERROR )

GOTO :END

:ERROR
ECHO.
ECHO %_fRed%%_bBlack%Some error occur, check CMake log.%_ResetColor%
PAUSE
EXIT /b 2

:END
ECHO.
ECHO %_fGreen%%_bBlack%Done!%_ResetColor%
ECHO.
