import json
import re

# --- CONFIGURATION ---
INPUT_DATASET = "C:\\Users\\Hank47\\Downloads\\archive\\arxiv-metadata-oai-snapshot.json" # Change this to your full dataset path
ID_MAP_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\id_map.txt"
OUTPUT_META_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_metadata.txt"

def normalize_id(raw_id):
    """
    Converts 'arXiv:0704.0001v1' -> '0704.0001'
    Matches the logic used by your C++ Graph Parser.
    """
    if not isinstance(raw_id, str): return str(raw_id)
    
    # 1. Remove 'arXiv:' prefix if present
    clean = raw_id.replace("arXiv:", "").strip()
    
    # 2. Remove version suffix (v1, v2, etc.)
    # This splits at 'v' and takes the first part if the suffix is numeric
    if "v" in clean:
        parts = clean.rsplit("v", 1)
        if parts[-1].isdigit():
            clean = parts[0]
            
    return clean

def clean_text(text):
    """Sanitizes text for the pipe-delimited file."""
    if not text: return "Unknown"
    # Remove pipes, newlines, tabs, and truncate extremely long titles
    text = str(text).replace('|', '-').replace('\n', ' ').replace('\r', '').replace('\t', ' ')
    return text.strip()

def main():
    print("--- Starting Robust Metadata Extraction ---")
    
    # 1. Load ID Map
    print(f"Loading {ID_MAP_FILE}...")
    str_to_int = {}
    try:
        with open(ID_MAP_FILE, 'r', encoding='utf-8') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) >= 2:
                    # parts[0] is the StringID (e.g., 0704.0001)
                    # parts[1] is the IntID (e.g., 5)
                    str_to_int[parts[0]] = int(parts[1])
    except FileNotFoundError:
        print("Error: id_map.txt missing. Run graph_parser.py first.")
        return

    print(f"Loaded {len(str_to_int)} mapped IDs.")
    
    # 2. Extract Metadata
    print(f"Scanning {INPUT_DATASET}...")
    
    # We use a dictionary to store results because the JSON is not sorted by IntID
    meta_store = {}
    matches_found = 0
    lines_processed = 0

    try:
        with open(INPUT_DATASET, 'r', encoding='utf-8') as f:
            for line in f:
                lines_processed += 1
                if not line.strip(): continue
                
                try:
                    data = json.loads(line)
                    raw_id = data.get('id', '')
                    
                    # --- THE FIX IS HERE ---
                    clean_pid = normalize_id(raw_id)
                    
                    if clean_pid in str_to_int:
                        int_id = str_to_int[clean_pid]
                        
                        # Extract Fields
                        title = clean_text(data.get('title', 'Untitled'))
                        authors = clean_text(data.get('authors', 'Unknown Authors'))
                        categories = clean_text(data.get('categories', 'N/A'))
                        date = clean_text(data.get('update_date', 'N/A'))
                        
                        # Format: ID|Title|Authors|Category|Date
                        meta_store[int_id] = f"{clean_pid}|{title}|{authors}|{categories}|{date}"
                        matches_found += 1
                        
                except json.JSONDecodeError:
                    continue
                
                if lines_processed % 10000 == 0:
                    print(f"Processed {lines_processed} lines... (Matches: {matches_found})", end='\r')

    except FileNotFoundError:
        print(f"\nError: {INPUT_DATASET} not found.")
        return

    print(f"\nScanning complete. Found metadata for {matches_found} documents.")

    # 3. Write Output Sorted by IntID
    # This ensures line 0 = DocID 0, line 1 = DocID 1, etc.
    print(f"Saving to {OUTPUT_META_FILE}...")
    
    if not str_to_int:
        print("Map was empty!")
        return

    max_id = max(str_to_int.values())
    
    with open(OUTPUT_META_FILE, 'w', encoding='utf-8') as f:
        for i in range(max_id + 1):
            if i in meta_store:
                f.write(meta_store[i] + "\n")
            else:
                # Placeholder for IDs that exist in the graph but weren't found in this JSON scan
                f.write(f"UnknownID|Unknown Title (Doc #{i})|Unknown|N/A|N/A\n")

    print("Success! Now run your search engine.")

if __name__ == "__main__":
    main()