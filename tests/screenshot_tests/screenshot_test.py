#!/usr/bin/env python3
"""
Screenshot regression test for ExperiRender
Tests the default viewport display without models loaded
"""

import os
import sys
import time
import subprocess
import shutil
from pathlib import Path
import flip_evaluator as flip
from PIL import Image

class ScreenshotTest:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent.parent  # Go up to ExperiRender root
        self.baselines_dir = self.project_root / "baselines"
        self.testing_dir = self.project_root / "build" / "testing"
        self.renderer_exe = self.project_root / "build" / "Debug" / "Renderer.exe"
        
        # Test configuration
        self.test_name = "TEST_DEFAULT_VIEWPORT_ON_LAUNCH"
        self.baseline_file = self.baselines_dir / f"{self.test_name}_baseline.png"
        self.current_file = self.testing_dir / f"{self.test_name}_current.png"
        self.diff_file = self.testing_dir / f"{self.test_name}_diff.png"
        
        # FLIP parameters
        self.flip_threshold = 0.1  # Adjust as needed
        
    def setup_directories(self):
        """Create necessary directories"""
        self.baselines_dir.mkdir(exist_ok=True)
        self.testing_dir.mkdir(exist_ok=True)
        
    def check_renderer_exists(self):
        """Check if renderer executable exists"""
        if not self.renderer_exe.exists():
            print(f"ERROR: Renderer executable not found at {self.renderer_exe}")
            print("Please build the project first")
            return False
        return True
        
    def capture_screenshot(self, automated=True):
        """
        Launch renderer and capture screenshot
        Returns path to captured screenshot or None if failed
        """
        print("Launching renderer...")
        
        # Launch renderer process
        try:
            process = subprocess.Popen(
                [str(self.renderer_exe)],
                cwd=str(self.project_root / "build")
            )
            
            # Wait for renderer to initialize
            print("Waiting for renderer to initialize...")
            time.sleep(5)  # Increased wait time for full initialization
            
            # Check if process is still running
            if process.poll() is not None:
                print(f"ERROR: Renderer exited prematurely")
                return None
            
            print("Renderer launched successfully")
            
            # Manual screenshot capture (for now)
            print("Please press F12 to capture screenshot when ready...")
            print("Then close the renderer window...")
            process.wait()
            
            # Look for the most recent screenshot file in build directory
            build_dir = self.project_root / "build"
            screenshot_files = list(build_dir.glob("screenshot_*.png"))
            if not screenshot_files:
                print("ERROR: No screenshot files found")
                return None
                
            # Get the most recent screenshot
            latest_screenshot = max(screenshot_files, key=lambda p: p.stat().st_mtime)
            print(f"Found screenshot: {latest_screenshot}")
            
            # Copy to testing directory
            shutil.copy2(latest_screenshot, self.current_file)
            print(f"Screenshot saved to: {self.current_file}")
            
            # Clean up original screenshot
            latest_screenshot.unlink()
            
            return self.current_file
            
        except Exception as e:
            print(f"ERROR: Failed to capture screenshot: {e}")
            return None
    
    def create_baseline(self):
        """Create baseline image from current screenshot"""
        if not self.current_file.exists():
            print(f"ERROR: No current screenshot to use as baseline for {self.test_name}")
            return False
            
        shutil.copy2(self.current_file, self.baseline_file)
        print(f"Baseline created for {self.test_name}: {self.baseline_file}")
        return True
        
    def compare_images(self):
        """Compare current screenshot with baseline using FLIP"""
        if not self.baseline_file.exists():
            print(f"ERROR: No baseline image found for {self.test_name}")
            print("Run with --create-baseline to create one")
            return False
            
        if not self.current_file.exists():
            print(f"ERROR: No current screenshot found for {self.test_name}")
            return False
            
        print(f"Comparing images with NVIDIA FLIP for {self.test_name}...")
        
        try:
            # Try using FLIP command line tool instead of Python API
            import subprocess
            import re
            
            # Run FLIP command line tool
            cmd = [
                "flip", 
                "-r", str(self.baseline_file),
                "-t", str(self.current_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=str(self.project_root))
            
            if result.returncode == 0:
                # Parse the mean error from output
                output = result.stdout
                print("FLIP command line output:")
                print(output)
                
                # Extract mean error value
                mean_match = re.search(r'Mean:\s+(\d+\.\d+)', output)
                if mean_match:
                    mean_error = float(mean_match.group(1))
                    print(f"FLIP mean error: {mean_error:.6f}")
                    
                    # Find the generated difference image
                    flip_output_match = re.search(r'FLIP error map location:\s+(.*\.png)', output)
                    if flip_output_match:
                        flip_diff_file = self.project_root / flip_output_match.group(1).strip()
                        if flip_diff_file.exists():
                            # Copy FLIP's output to our expected location
                            shutil.copy2(flip_diff_file, self.diff_file)
                            print(f"Difference image saved: {self.diff_file}")
                            # Clean up FLIP's output file
                            flip_diff_file.unlink()
                    
                    # Determine pass/fail
                    if mean_error <= self.flip_threshold:
                        print("✅ TEST PASSED - Images are similar enough")
                        return True
                    else:
                        print("❌ TEST FAILED - Images differ too much")
                        print(f"Error {mean_error:.6f} exceeds threshold {self.flip_threshold}")
                        return False
                else:
                    print("Could not parse mean error from FLIP output")
                    return False
            else:
                print("❌ FLIP command failed")
                print(f"FLIP output: {result.stdout}")
                if result.stderr:
                    print(f"FLIP errors: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"FLIP command line failed: {e}")
            print("Falling back to basic pixel comparison...")
            
            # Fallback to basic image comparison using PIL
            try:
                from PIL import Image, ImageChops
                import numpy as np
                
                # Load images
                ref_img = Image.open(self.baseline_file)
                test_img = Image.open(self.current_file)
                
                # Ensure same size
                if ref_img.size != test_img.size:
                    print(f"ERROR: Image sizes differ - Reference: {ref_img.size}, Test: {test_img.size}")
                    return False
                
                # Calculate difference
                diff = ImageChops.difference(ref_img, test_img)
                
                # Convert to numpy for analysis
                diff_array = np.array(diff)
                
                # Calculate mean absolute difference (0-255 scale)
                mean_diff = np.mean(diff_array) / 255.0
                
                # Save difference image
                diff.save(self.diff_file)
                
                print(f"Mean pixel difference: {mean_diff:.6f}")
                print(f"Difference image saved: {self.diff_file}")
                
                # Determine pass/fail
                if mean_diff <= self.flip_threshold:
                    print("✅ TEST PASSED - Images are similar enough")
                    return True
                else:
                    print("❌ TEST FAILED - Images differ too much")
                    print(f"Difference {mean_diff:.6f} exceeds threshold {self.flip_threshold}")
                    return False
                    
            except Exception as e2:
                print(f"ERROR: Fallback comparison failed: {e2}")
                return False
    
    def run_test(self, create_baseline=False):
        """Run the complete test"""
        print("=== ExperiRender Screenshot Test ===")
        print(f"Running test: {self.test_name}")
        
        # Setup
        self.setup_directories()
        
        if not self.check_renderer_exists():
            return False
            
        # Capture screenshot
        screenshot_path = self.capture_screenshot(automated=self.automated_mode)
        if not screenshot_path:
            return False
            
        # Create baseline if requested
        if create_baseline:
            result = self.create_baseline()
            if result:
                print(f"Baseline created successfully for test: {self.test_name}")
            return result
            
        # Compare with baseline
        result = self.compare_images()
        if result:
            print(f"Test PASSED: {self.test_name}")
        else:
            print(f"Test FAILED: {self.test_name}")
        return result

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Screenshot regression test for ExperiRender")
    parser.add_argument("--create-baseline", action="store_true",
                       help="Create baseline image instead of comparing")
    parser.add_argument("--threshold", type=float, default=0.1,
                       help="FLIP error threshold (default: 0.1)")
    parser.add_argument("--manual", action="store_true",
                       help="Use manual F12 capture instead of automatic")
    
    args = parser.parse_args()
    
    test = ScreenshotTest()
    test.flip_threshold = args.threshold
    test.automated_mode = not (args.manual or args.create_baseline)
    
    success = test.run_test(create_baseline=args.create_baseline)
    
    if success:
        print("\n✅ Test completed successfully")
        return 0
    else:
        print("\n❌ Test failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())