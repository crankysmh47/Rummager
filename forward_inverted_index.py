import os
import pickle

from collections import defaultdict
from build_lexicon import tokenize

# ---------------------------------------------------------
# Forward Index
# ---------------------------------------------------------

# ---------------------------------------------------------
# Forward Index
# ---------------------------------------------------------
"""
Forward index structure:

forward_index = {
    doc_id: [(word_id, position), (word_id, position), ...]
}
"""

def build_forward_index(articles, lexicon):
    forward_index = {}
    
    for art in articles:
        doc_id = art["id"]
        tokens = tokenize(art["text"])
        
        entries = []
        pos = 0
        
        for token in tokens:
            if token in lexicon:
                word_id = lexicon[token]
                entries.append((word_id, pos))
                pos += 1
        
        forward_index[doc_id] = entries
    
    return forward_index

def save_forward_index(forward_index, file_path):
    """
    Format to save:
    docID   wordID:pos,wordID:pos,wordID:pos,...
    """
    with open(file_path, "w", encoding="utf-8") as f:
        for doc_id, entries in forward_index.items():
            pairs = [f"{wid}:{pos}" for (wid, pos) in entries]
            f.write(f"{doc_id}\t{','.join(pairs)}\n")

def save_forward_index_binary(forward_index, file_path):
    with open(file_path, 'wb') as f:
        pickle.dump(forward_index, f)

def load_forward_index_binary(file_path):
    with open(file_path, 'rb') as f:
        return pickle.load(f)


# ---------------------------------------------------------
# Inverted Index
# ---------------------------------------------------------
"""
Inverted index structure:

inverted_index = {
    word_id: {
        doc_id: [pos1, pos2, pos3...]
    }
}
"""

def create_inner_defaultdict():
    return defaultdict(list)

def build_inverted_index(forward_index):
    inverted_index = defaultdict(create_inner_defaultdict)
    
    for doc_id, entries in forward_index.items():
        for word_id, pos in entries:
            inverted_index[word_id][doc_id].append(pos)
    
    return inverted_index

def save_inverted_index(inverted_index, file_path):
    """
    Format to save:
    wordID   docID:pos,pos,pos;docID:pos,pos;...
    """
    with open(file_path, "w", encoding="utf-8") as f:
        for word_id, doc_map in inverted_index.items():
            doc_entries = []
            for doc_id, positions in doc_map.items():
                pos_str = ",".join(str(p) for p in positions)
                doc_entries.append(f"{doc_id}:{pos_str}")
            
            f.write(f"{word_id}\t{';'.join(doc_entries)}\n")

def save_inverted_index_binary(inverted_index, file_path):
    with open(file_path, 'wb') as f:
        pickle.dump(inverted_index, f)

def load_inverted_index_binary(file_path):
    with open(file_path, 'rb') as f:
        return pickle.load(f)


# ---------------------------------------------------------
# Usage Example (called from main lexicon pipeline)
# ---------------------------------------------------------

if __name__ == "__main__":
    from build_lexicon import load_arxiv_dataset, build_lexicon  # <-- your lexicon file

    dataset_file = "../data/arxiv-metadata-oai-snapshot.json"
    articles = load_arxiv_dataset(dataset_file, limit=20000)

    # Build or load lexicon
    lexicon = build_lexicon(articles)

    # Forward index
    forward_index = build_forward_index(articles, lexicon)
    save_forward_index(forward_index, "../indexes/forward_index.txt")
    print("Forward index saved.")

    # Inverted index
    inverted_index = build_inverted_index(forward_index)
    save_inverted_index(inverted_index, "../indexes/inverted_index.txt")
    print("Inverted index saved.")
