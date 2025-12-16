import json
import sys

# --- CONFIGURATION ---
MAP_FILE = "id_map.txt"             # Input: StringID -> IntID
SCORES_FILE = "pagerank_scores.txt" # Input: IntID -> Score
OUTPUT_FILE = "arxiv_pagerank.json" # Output: StringID -> Score

def merge_results():
    print("Step 1: Loading Scores into Memory...")
    # We use a dictionary to map IntID -> Score. 
    # If IDs are strictly 0..N, a list would be faster, but dict is safer.
    int_to_score = {}
    
    try:
        with open(SCORES_FILE, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) != 2: continue
                
                # parts[0] is IntID, parts[1] is Score
                int_id = int(parts[0])
                score = float(parts[1])
                int_to_score[int_id] = score
                
        print(f"Loaded {len(int_to_score)} scores.")
        
    except FileNotFoundError:
        print(f"Error: Could not find {SCORES_FILE}. Run the C++ code first.")
        return

    print("Step 2: Merging IDs with Scores...")
    
    # We will write the output as a stream to avoid building a huge JSON object in RAM
    merged_count = 0
    
    try:
        with open(MAP_FILE, 'r', encoding='utf-8') as map_f, \
             open(OUTPUT_FILE, 'w', encoding='utf-8') as out_f:
            
            # Start valid JSON
            out_f.write("{\n")
            
            first_line = True
            
            for line in map_f:
                parts = line.strip().split()
                if len(parts) < 2: continue
                
                # In id_map.txt: "arXiv:1234.5678 502"
                str_id = parts[0]
                int_id = int(parts[1])
                
                # Retrieve score
                if int_id in int_to_score:
                    score = int_to_score[int_id]
                    
                    # JSON formatting handling (comma on all lines except the first)
                    if not first_line:
                        out_f.write(",\n")
                    first_line = False
                    
                    # Write entry: "ID": Score
                    out_f.write(f'  "{str_id}": {score:.10f}')
                    merged_count += 1
                
                if merged_count % 10000 == 0:
                    print(f"Merged {merged_count} papers...", end='\r')

            # Close JSON
            out_f.write("\n}")
            
        print(f"\nDone! Successfully merged {merged_count} papers.")
        print(f"Output saved to: {OUTPUT_FILE}")

    except FileNotFoundError:
        print(f"Error: Could not find {MAP_FILE}. Run the Python graph builder first.")

if __name__ == "__main__":
    merge_results()
