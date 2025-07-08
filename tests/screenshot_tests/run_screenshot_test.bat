@echo off
REM Screenshot regression test runner for ExperiRender
REM Usage: run_screenshot_test.bat [--create-baseline] [--threshold 0.1]

echo === ExperiRender Screenshot Test Runner ===

REM Check if virtual environment exists (look in project root)
if not exist "..\..\venv_screenshot_test\Scripts\activate.bat" (
    echo ERROR: Virtual environment not found
    echo Please run from project root: python -m venv venv_screenshot_test
    echo Then: venv_screenshot_test\Scripts\activate
    echo And: pip install flip-evaluator pillow psutil numpy
    pause
    exit /b 1
)

REM Activate virtual environment
call ..\..\venv_screenshot_test\Scripts\activate.bat

REM Run the test
python screenshot_test.py %*

REM Deactivate virtual environment
call ..\..\venv_screenshot_test\Scripts\deactivate.bat

pause