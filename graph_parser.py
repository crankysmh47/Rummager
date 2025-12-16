import json
import sys

# --- CONFIGURATION ---
# Ensure this points to your CITATION file (internal-references)
INPUT_FILE = r"D:\sem 3 labs\Data\internal-references-pdftotext.json"
MAP_FILE = "id_map.txt"
GRAPH_FILE = "graph.txt"

def clean_id(raw_id):
    # Helper to strip versions like 'v1' from ids like '0704.0001v1'
    if not isinstance(raw_id, str): return str(raw_id)
    if raw_id.startswith("arXiv:"): raw_id = raw_id[6:]
    if "v" in raw_id:
        parts = raw_id.rsplit("v", 1)
        if parts[-1].isdigit(): 
            return parts[0]
    return raw_id

def parse_and_convert():
    print(f"Opening {INPUT_FILE}...")
    
    # --- PASS 1: MAP IDs ---
    print("PASS 1: Indexing all IDs...")
    unique_ids = set()
    
    try:
        with open(INPUT_FILE, 'r', encoding='utf-8') as f:
            for i, line in enumerate(f):
                if not line.strip(): continue
                try:
                    data = json.loads(line)
                except json.JSONDecodeError:
                    continue
                
                # The Fix: Iterate over ALL items in the dictionary
                if isinstance(data, dict):
                    for src, refs in data.items():
                        # Add the Source ID
                        unique_ids.add(clean_id(src))
                        # Add all Target IDs
                        for r in refs:
                            unique_ids.add(clean_id(r))
                            
                if i % 100 == 0:
                    print(f"Scanned {i} lines... (Found {len(unique_ids)} papers)", end='\r')
                    
    except FileNotFoundError:
        print(f"\nError: Could not find {INPUT_FILE}")
        return

    print(f"\nSorting {len(unique_ids)} IDs...")
    sorted_ids = sorted(list(unique_ids))
    str_to_int = {s_id: i for i, s_id in enumerate(sorted_ids)}
    
    # Free memory
    del unique_ids
    
    print(f"Saving {MAP_FILE}...")
    with open(MAP_FILE, 'w', encoding='utf-8') as f:
        for s_id in sorted_ids:
            f.write(f"{s_id} {str_to_int[s_id]}\n")
            
    # --- PASS 2: BUILD GRAPH ---
    print("PASS 2: Building Integer Graph...")
    
    with open(GRAPH_FILE, 'w', encoding='utf-8') as out_f:
        out_f.write(f"{len(str_to_int)}\n") # Header
        
        with open(INPUT_FILE, 'r', encoding='utf-8') as in_f:
            for i, line in enumerate(in_f):
                if not line.strip(): continue
                try:
                    data = json.loads(line)
                except: continue
                
                if isinstance(data, dict):
                    # Iterate over ALL papers in this chunk
                    for src, refs in data.items():
                        src_clean = clean_id(src)
                        
                        # Verify ID exists (it should from Pass 1)
                        if src_clean not in str_to_int: continue
                        
                        src_int = str_to_int[src_clean]
                        
                        # Convert references to integers
                        target_ints = []
                        for r in refs:
                            r_clean = clean_id(r)
                            if r_clean in str_to_int:
                                target_ints.append(str_to_int[r_clean])
                        
                        # Write to file
                        if not target_ints:
                            out_f.write(f"{src_int} 0\n")
                        else:
                            # Format: Source OutDegree Target1 Target2 ...
                            line_parts = [str(src_int), str(len(target_ints))] + [str(t) for t in target_ints]
                            out_f.write(" ".join(line_parts) + "\n")

                if i % 100 == 0:
                    print(f"Processed {i} lines...", end='\r')

    print(f"\nSuccess! Created {GRAPH_FILE} and {MAP_FILE}.")

if __name__ == "__main__":
    parse_and_convert()