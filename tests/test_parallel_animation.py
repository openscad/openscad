#!/usr/bin/env python3
import sys
import subprocess
import time
import os
import filecmp

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path_to_openscad_binary>")
        sys.exit(1)
        
    openscad_bin = sys.argv[1]
    
    # 1. Write the test .scad file
    # A heavy static object (sphere with many cutouts) and a light dynamic object (cube)
    scad_code = """
$fn = 100;


// Heavy dynamic object
render() {
    minkowski() {
        sphere(r=10 + $t*5, $fn=50);
        cube([10 + $t*10, 10, 10], center=true);
    }
}
"""
    scad_file = "test_anim_temp.scad"
    with open(scad_file, "w") as f:
        f.write(scad_code)
        
    frames = 10
    
    # 2. Run sequential generation
    print("Running sequential animation generation...")
    seq_start = time.time()
    subprocess.run([
        openscad_bin, 
        "-o", "out_seq.png", 
        "--animate", str(frames), 
        scad_file
    ], check=True)
    seq_end = time.time()
    seq_time = seq_end - seq_start
    print(f"Sequential time: {seq_time:.2f} seconds")
    
    # 3. Run parallel generation
    print("\nRunning parallel animation generation...")
    par_start = time.time()
    res = subprocess.run([
        openscad_bin, 
        "-o", "out_par.png", 
        "--animate", str(frames), 
        "--animate-threads", "4",
        scad_file
    ])
    par_end = time.time()
    
    if res.returncode != 0:
        print("\nFAIL: Parallel run failed. (Expected if flag is not implemented yet)")
        sys.exit(1)
        
    par_time = par_end - par_start
    print(f"Parallel time: {par_time:.2f} seconds")
    
    # 4. Compare outputs
    print("\nVerifying outputs...")
    for i in range(frames):
        seq_img = f"out_seq{i:05d}.png"
        par_img = f"out_par{i:05d}.png"
        
        if not os.path.exists(par_img):
            print(f"FAIL: Expected parallel output {par_img} not found.")
            sys.exit(1)
            
        if not filecmp.cmp(seq_img, par_img, shallow=False):
            print(f"FAIL: Frame {i} differs between sequential and parallel generation.")
            sys.exit(1)
            
    print("SUCCESS: All parallel frames perfectly match sequential frames.")
    
    # 5. Check performance scaling
    # We expect some overhead, but it should be significantly faster than sequential.
    # A perfectly linear scale would be seq_time / 4, but let's just assert it's at least 1.5x faster.
    speedup = seq_time / par_time
    print(f"\nSpeedup factor: {speedup:.2f}x")
    
    if speedup < 1.5:
        print("FAIL: Parallel execution did not meet the performance scaling target (>= 1.5x).")
        sys.exit(1)
        
    print("SUCCESS: Performance scaling target met.")
    sys.exit(0)

if __name__ == "__main__":
    main()
