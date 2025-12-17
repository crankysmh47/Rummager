import json
import sys


INPUT_FILE = "C:\\Users\\Hank47\\Downloads\\archive\\arxiv-metadata-oai-snapshot.json"
OUTPUT_FILE = "clean_dataset.txt"

def clean_text(text):

    if not text:
        return ""
    return text.replace('\n', ' ').replace('\t', ' ').replace('\r', ' ')

def main():
    print(f"Starting preprocessing of {INPUT_FILE}...")
    
    try:
        with open(INPUT_FILE, 'r', encoding='utf-8') as f_in, \
             open(OUTPUT_FILE, 'w', encoding='utf-8') as f_out:
            
            count = 0
            for line in f_in:
                if not line.strip(): continue

                try:
                    data = json.loads(line)
                    
                    # 1. Extract Fields
                    doc_id = data.get('id', 'Unknown')
                    title = clean_text(data.get('title', ''))
                    abstract = clean_text(data.get('abstract', ''))
                    authors = clean_text(data.get('authors', ''))
                    categories = clean_text(data.get('categories', ''))
                    update_date = clean_text(data.get('update_date', ''))
                    
                    # 2. Combine content for the Lexicon

                    full_content = f"{title} {authors} {abstract} {categories} {update_date}"
                    f_out.write(f"{doc_id}\t{full_content}\n")
                    
                    count += 1
                    if count % 10000 == 0:
                        print(f"Processed {count} documents...", end='\r')
                        
                except json.JSONDecodeError:
                    continue # Skip malformed lines
                    
            print(f"\nSuccess! Processed {count} documents.")
            print(f"Clean data saved to: {OUTPUT_FILE}")

    except FileNotFoundError:
        print(f"Error: Could not find file {INPUT_FILE}")

if __name__ == "__main__":
    main()





# preprocess.py (The Cleaner)
# Exact Function: This script reads the raw ArXiv JSON file (approx. 3.5GB+),
# extracts only the specific fields required for indexing (ID, Title, Abstract, Authors, Categories, Date),
# removes structural characters that would confuse a parser (newlines/tabs), and saves it as a flat .txt file.

# Key Optimizations:

# Streaming (for line in f_in):
# Why: It never loads the whole file into RAM. It reads one line, processes it, writes it, and forgets it. 
# This keeps memory usage extremely low (under 50MB), regardless of dataset size.

# json.loads (C-optimized):
# Why: Python’s built-in JSON library is written in C. 
# It handles complex edge cases (nested quotes, Unicode, escaped characters) faster and more reliably than a manual C++ string parser would.

# One-Write-Per-Doc:
# Why: We aggregate title, abstract, etc., into a single variable full_content and write to the disk once per document.
#  This reduces Disk I/O overhead compared to writing field-by-field.

# Error Suppression (try-except):

# Why: In a dataset of 2 million files, some will be corrupt.
#  This script skips bad lines without crashing, ensuring the C++ code never receives garbage.