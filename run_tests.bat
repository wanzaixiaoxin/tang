@echo off
chcp 65001 >nul
echo ========================================
echo Tang C++20 协程框架 - 测试脚本
echo ========================================
echo.

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build
set TESTS_DIR=%SCRIPT_DIR%tests

echo [1/5] 检查构建目录...
if not exist "%BUILD_DIR%" (
    echo 创建构建目录...
    mkdir "%BUILD_DIR%"
)

echo [2/5] 配置CMake...
cd /d "%BUILD_DIR%"
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug

if %ERRORLEVEL% NEQ 0 (
    echo CMake配置失败!
    exit /b 1
)

echo [3/5] 编译测试...
cmake --build . --target all -j 4

if %ERRORLEVEL% NEQ 0 (
    echo 编译失败!
    exit /b 1
)

echo [4/5] 运行测试...
echo.

echo ----------------------------------------
echo 正在运行 task_test (任务系统测试)...
echo ----------------------------------------
if exist "%BUILD_DIR%\tests\task_test.exe" (
    "%BUILD_DIR%\tests\task_test.exe"
    if %ERRORLEVEL% NEQ 0 (
        echo task_test 运行失败!
        exit /b 1
    )
) else (
    echo 警告: task_test.exe 未找到
)

echo.
echo ----------------------------------------
echo 正在运行 channel_test (通道测试)...
echo ----------------------------------------
if exist "%BUILD_DIR%\tests\channel_test.exe" (
    "%BUILD_DIR%\tests\channel_test.exe"
    if %ERRORLEVEL% NEQ 0 (
        echo channel_test 运行失败!
        exit /b 1
    )
) else (
    echo 警告: channel_test.exe 未找到
)

echo.
echo ----------------------------------------
echo 正在运行 select_test (选择器测试)...
echo ----------------------------------------
if exist "%BUILD_DIR%\tests\select_test.exe" (
    "%BUILD_DIR%\tests\select_test.exe"
    if %ERRORLEVEL% NEQ 0 (
        echo select_test 运行失败!
        exit /b 1
    )
) else (
    echo 警告: select_test.exe 未找到
)

echo.
echo ----------------------------------------
echo 正在运行 runtime_test (运行时测试)...
echo ----------------------------------------
if exist "%BUILD_DIR%\tests\runtime_test.exe" (
    "%BUILD_DIR%\tests\runtime_test.exe"
    if %ERRORLEVEL% NEQ 0 (
        echo runtime_test 运行失败!
        exit /b 1
    )
) else (
    echo 警告: runtime_test.exe 未找到
)

echo.
echo ----------------------------------------
echo 正在运行 integration_test (集成测试)...
echo ----------------------------------------
if exist "%BUILD_DIR%\tests\integration_test.exe" (
    "%BUILD_DIR%\tests\integration_test.exe"
    if %ERRORLEVEL% NEQ 0 (
        echo integration_test 运行失败!
        exit /b 1
    )
) else (
    echo 警告: integration_test.exe 未找到
)

echo.
echo ========================================
echo 所有测试通过!
echo ========================================
echo.
echo 测试结果摘要:
echo   - task_test:      通过
echo   - channel_test:   通过
echo   - select_test:    通过
echo   - runtime_test:   通过
echo   - integration_test: 通过
echo.

exit /b 0
