import os
import sys
import json
import struct

# Configuration
# DEFAULT_DATASET_PATH will be overridden if passed as argument
DEFAULT_DATASET_PATH = r"D:\\Web Downloads\\arxiv-metadata-oai-snapshot.json" 
CLEAN_FILE = "clean_dataset.txt"
BARRELS_DIR = "barrels"
NUM_DOCS = 5000

def clean_text(text):
    if not text: return ""
    return text.replace('\n', ' ').replace('\t', ' ').replace('\r', ' ')

def preprocess_partial(input_path, output_path, limit):
    print(f"Preprocessing first {limit} documents from {input_path}...")
    try:
        with open(input_path, 'r', encoding='utf-8') as f_in, \
             open(output_path, 'w', encoding='utf-8') as f_out:
            count = 0
            for line in f_in:
                if not line.strip(): continue
                try:
                    data = json.loads(line)
                    doc_id = data.get('id', 'Unknown')
                    title = clean_text(data.get('title', ''))
                    abstract = clean_text(data.get('abstract', ''))
                    authors = clean_text(data.get('authors', ''))
                    categories = clean_text(data.get('categories', ''))
                    update_date = clean_text(data.get('update_date', ''))
                    
                    full_content = f"{title} {authors} {abstract} {categories} {update_date}"
                    f_out.write(f"{doc_id}\t{full_content}\n")
                    
                    count += 1
                    if count >= limit: break
                except json.JSONDecodeError:
                    continue
        print(f"Created {output_path} with {count} documents.")
    except FileNotFoundError:
        print(f"Error: Dataset file not found at {input_path}")
        sys.exit(1)

def run_cpp(binary):
    print(f"Running {binary}...")
    if os.system(binary) != 0:
        print(f"Error running {binary}")
        sys.exit(1)

def read_barrel(barrel_path, txt_path):
    print(f"Converting {barrel_path} to {txt_path}...")
    try:
        with open(barrel_path, 'rb') as f_in, open(txt_path, 'w') as f_out:
            while True:
                # Read WordID (4 bytes)
                data = f_in.read(4)
                if not data: break
                word_id = struct.unpack('I', data)[0]
                
                # Read Count (4 bytes)
                data = f_in.read(4)
                count = struct.unpack('I', data)[0]
                
                f_out.write(f"WordID: {word_id}, Count: {count}\n")
                
                # Read Postings (count * 8 bytes)
                # Posting = DocID (4) + Freq (4)
                for _ in range(count):
                    p_data = f_in.read(8)
                    if len(p_data) < 8: break
                    doc_id, freq = struct.unpack('II', p_data)
                    f_out.write(f"  DocID: {doc_id}, Freq: {freq}\n")
    except Exception as e:
        print(f"Error reading barrel: {e}")

def main():
    dataset_path = DEFAULT_DATASET_PATH
    if len(sys.argv) > 1:
        dataset_path = sys.argv[1]
        
    if "INSERT_PATH" in dataset_path or not os.path.exists(dataset_path):
        print(f"Error: Invalid dataset path: {dataset_path}")
        print("Please provide the absolute path to 'arxiv-metadata-oai-snapshot.json' as an argument.")
        sys.exit(1)

    # 1. Preprocess
    preprocess_partial(dataset_path, CLEAN_FILE, NUM_DOCS)
    
    # 2. Run Pipeline
    run_cpp("build_lexicon.exe")
    run_cpp("forward_indexer.exe")
    run_cpp("invert.exe")
    
    if os.path.exists(BARRELS_DIR):
        for f in os.listdir(BARRELS_DIR):
            os.remove(os.path.join(BARRELS_DIR, f))
    else:
        os.makedirs(BARRELS_DIR)
        
    run_cpp("barrels.exe")
    
    # 3. Verify Barrels
    if not os.path.exists(BARRELS_DIR):
        print("Error: Barrels directory not created.")
        sys.exit(1)
        
    for filename in os.listdir(BARRELS_DIR):
        if filename.endswith(".bin"):
            bin_path = os.path.join(BARRELS_DIR, filename)
            txt_path = os.path.join(BARRELS_DIR, filename.replace(".bin", ".txt"))
            read_barrel(bin_path, txt_path)
            
    print("Verification complete. Check barrels/ folder for .txt files.")

if __name__ == "__main__":
    main()
