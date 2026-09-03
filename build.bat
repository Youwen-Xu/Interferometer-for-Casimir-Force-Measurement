@echo off
setlocal
chcp 65001 >nul

set "PROJECT_DIR=%~dp0"
set "GCC=C:\Strawberry\c\bin\gcc.exe"
set "WINDRES=C:\Strawberry\c\bin\windres.exe"

if not exist "%GCC%" (
    echo 未找到编译器：%GCC%
    echo 请确认 Strawberry Perl / MinGW-W64 已安装。
    pause
    exit /b 1
)

if not exist "%PROJECT_DIR%build" mkdir "%PROJECT_DIR%build"

"%WINDRES%" -i "%PROJECT_DIR%app.rc" -o "%PROJECT_DIR%build\app.res.o"
if errorlevel 1 goto :failed

"%GCC%" -std=c11 -Wall -Wextra -Werror -O2 -finput-charset=UTF-8 ^
    -I "%PROJECT_DIR%src" ^
    "%PROJECT_DIR%src\main.c" "%PROJECT_DIR%src\motion_logic.c" "%PROJECT_DIR%src\ametek.c" ^
    "%PROJECT_DIR%build\app.res.o" ^
    -o "%PROJECT_DIR%NanoStageControl.exe" ^
    -municode -mwindows -lcomctl32 -lcomdlg32 -lgdi32 -lwinhttp -lm
if errorlevel 1 goto :failed

"%GCC%" -std=c11 -Wall -Wextra -Werror -O2 -finput-charset=UTF-8 ^
    -I "%PROJECT_DIR%src" ^
    "%PROJECT_DIR%tests\test_motion_logic.c" "%PROJECT_DIR%src\motion_logic.c" ^
    -o "%PROJECT_DIR%build\test_motion_logic.exe" -lm
if errorlevel 1 goto :failed

"%PROJECT_DIR%build\test_motion_logic.exe"
if not "%ERRORLEVEL%"=="0" goto :failed

"%GCC%" -std=c11 -Wall -Wextra -Werror -O2 -finput-charset=UTF-8 ^
    -I "%PROJECT_DIR%src" ^
    "%PROJECT_DIR%tests\test_ametek.c" "%PROJECT_DIR%src\ametek.c" ^
    -o "%PROJECT_DIR%build\test_ametek.exe" -lwinhttp -lm
if errorlevel 1 goto :failed

"%PROJECT_DIR%build\test_ametek.exe"
if not "%ERRORLEVEL%"=="0" goto :failed

echo.
echo 构建与离线测试均已通过。
exit /b 0

:failed
echo.
echo 构建或测试失败。
pause
exit /b 1
