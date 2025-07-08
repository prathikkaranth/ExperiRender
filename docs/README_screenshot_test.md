# Screenshot Regression Testing

This document describes the screenshot regression testing system for ExperiRender using NVIDIA FLIP for perceptual image comparison.

Test scripts are located in `tests/screenshot_tests/`.

## Setup

### 1. Create Python Virtual Environment (from project root)
```cmd
cd /path/to/ExperiRender
python -m venv venv_screenshot_test
venv_screenshot_test\Scripts\activate
```

### 2. Install Required Packages
```cmd
pip install flip-evaluator pillow psutil numpy
```

### 3. Build ExperiRender
Make sure `Renderer.exe` exists in the `build/Debug/` directory.

## Usage

### Create Baseline (First Time)
1. Navigate to the test directory and run:
   ```cmd
   cd tests/screenshot_tests
   run_screenshot_test.bat --create-baseline
   ```
2. The renderer will launch automatically
3. When ready, press **F12** to capture the screenshot
4. Close the renderer window
5. The baseline image will be saved in `baselines/default_viewport_baseline.png`

### Run Regression Test
1. Navigate to the test directory and run:
   ```cmd
   cd tests/screenshot_tests
   run_screenshot_test.bat
   ```
2. The renderer will launch automatically
3. When ready, press **F12** to capture the screenshot
4. Close the renderer window
5. The test will compare against the baseline and show results

### Advanced Options
- Custom threshold: `run_screenshot_test.bat --threshold 0.05`
- Direct Python execution: `python screenshot_test.py --help`
- All commands should be run from `tests/screenshot_tests/` directory

## Test Results

### Pass ✅
- FLIP error is below threshold (default: 0.1)
- Images are perceptually similar

### Fail ❌
- FLIP error exceeds threshold
- Check `build/testing/default_viewport_diff.png` for visual differences

## Files Structure

```
/baselines/                           
  └── default_viewport_baseline.png  
/build/testing/                       
  ├── default_viewport_current.png   
  └── default_viewport_diff.png      
/tests/screenshot_tests/              
  ├── screenshot_test.py              
  └── run_screenshot_test.bat         
/docs/                                
  └── README_screenshot_test.md      
```

## Troubleshooting

### Renderer Not Found
- Ensure Renderer.exe is built in the `build/Debug/` directory
- Check that the build was successful

### No Screenshot Captured
- Make sure to press **F12** while the renderer is running
- Wait for the renderer to fully initialize (3+ seconds)
- Check that the renderer window has focus when pressing F12

### Python Import Errors
- Ensure virtual environment is activated
- Verify packages are installed: `pip list`
- Try reinstalling: `pip install --force-reinstall flip-evaluator`

