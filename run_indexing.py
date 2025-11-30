import os
import sys
import argparse
import json
from build_lexicon import load_arxiv_dataset, build_lexicon, save_lexicon, save_lexicon_binary
from forward_inverted_index import build_forward_index, build_inverted_index, save_forward_index, save_inverted_index, save_forward_index_binary, save_inverted_index_binary

def main():
    # ---------------------------------------------------------
    # Configuration
    # ---------------------------------------------------------
    DATASET_PATH = r"D:\Web Downloads\arxiv-metadata-oai-snapshot.json"  # <--- PASTE YOUR PATH HERE
    LIMIT = 1000
    OUTPUT_DIR = "output"
    # ---------------------------------------------------------

    if not os.path.exists(DATASET_PATH):
        print(f"Error: Dataset file not found at {DATASET_PATH}")
        print("Please edit the DATASET_PATH variable in the script.")
        return

    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    print(f"Loading dataset from {DATASET_PATH} (limit={LIMIT})...")
    articles = load_arxiv_dataset(DATASET_PATH, limit=LIMIT)
    
    # Save sample articles
    sample_articles_path = os.path.join(OUTPUT_DIR, "sample_articles.json")
    with open(sample_articles_path, "w", encoding="utf-8") as f:
        for art in articles:
            f.write(json.dumps(art) + "\n")
    print(f"Sample articles saved to {sample_articles_path}")

    print("Building lexicon...")
    lexicon = build_lexicon(articles)
    lexicon_path = os.path.join(OUTPUT_DIR, "lexicon.txt")
    save_lexicon(lexicon, lexicon_path)
    save_lexicon_binary(lexicon, lexicon_path.replace('.txt', '.pkl'))
    print(f"Lexicon saved to {lexicon_path} (Total words: {len(lexicon)})")

    print("Building forward index...")
    forward_index = build_forward_index(articles, lexicon)
    forward_index_path = os.path.join(OUTPUT_DIR, "forward_index.txt")
    save_forward_index(forward_index, forward_index_path)
    save_forward_index_binary(forward_index, forward_index_path.replace('.txt', '.pkl'))
    print(f"Forward index saved to {forward_index_path}")

    print("Building inverted index...")
    inverted_index = build_inverted_index(forward_index)
    inverted_index_path = os.path.join(OUTPUT_DIR, "inverted_index.txt")
    save_inverted_index(inverted_index, inverted_index_path)
    save_inverted_index_binary(inverted_index, inverted_index_path.replace('.txt', '.pkl'))
    print(f"Inverted index saved to {inverted_index_path}")

    print("Done.")

if __name__ == "__main__":
    main()
