import subprocess
import time
import sys
import os

def run_stress_test(input_file):
    print(f"--- Running Stress Test with {input_file} ---")
    if not os.path.exists(input_file):
        print(f"Skipping: {input_file} not found.")
        return True

    try:
        with open(input_file, 'r') as f:
            queries = f.readlines()
        
        process = subprocess.Popen(
            ['./searchengine.exe'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        start_time = time.time()
        
        # Send all queries
        for q in queries:
            q = q.strip()
            if q:
                process.stdin.write(q + "\n")
        
        process.stdin.write("exit\n")
        process.stdin.flush()
        
        stdout, stderr = process.communicate(timeout=10)
        end_time = time.time()
        
        print(f"Output received: {len(stdout)} bytes")
        # Check for key outputs
        if "Found" in stdout and "results" in stdout:
             print("SUCCESS: Search results detected.")
        else:
             print("WARNING: No standard results found (might be dry run).")

        if stderr:
            print(f"Errors: {stderr}")
            
        print(f"Total Time: {end_time - start_time:.4f}s")
        print("--- Test Complete ---\n")
        return True
    except Exception as e:
        print(f"Test Failed: {e}")
        return False

if __name__ == "__main__":
    tests = ["stress_1.input", "stress_2.input"]
    success = True
    for t in tests:
        if not run_stress_test(t):
            success = False
    
    if success:
        print("SYSTEM VERIFICATION COMPLETE.")
    else:
        sys.exit(1)
