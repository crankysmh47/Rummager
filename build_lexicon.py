import json
from tqdm import tqdm  # optional, for progress bar
from nltk.tokenize import word_tokenize
from nltk.corpus import stopwords
import string

def load_arxiv_dataset(file_path, limit=None):
    """
    Reads the dataset JSONL file line by line.
    Returns a list of dicts: [{"id":..., "text":...}, ...]
    """
    articles = []
    count = 0

    with open(file_path, 'r', encoding='utf-8') as f:
        for line in tqdm(f, total=limit):  # progress bar
            obj = json.loads(line)
            paper_id = obj.get("id")
            # Combine fields into one string
            text = obj.get("title", "") + " " + obj.get("abstract", "") + " " + obj.get("categories", "")
            articles.append({"id": paper_id, "text": text})
            count += 1
            if limit and count >= limit:  # only load first N papers
                break

    return articles

stop_words = set(stopwords.words("english"))
translator = str.maketrans('', '', string.punctuation)

def tokenize(text):
    # lowercase
    text = text.lower()
    # remove punctuation
    text = text.translate(translator)
    # tokenize
    tokens = word_tokenize(text)
    # remove stopwords and non-alphabetic tokens
    tokens = [t for t in tokens if t.isalpha() and t not in stop_words]
    return tokens

def build_lexicon(articles):
    """
    Build lexicon: word -> unique ID
    """
    lexicon = {}
    current_id = 1

    for art in articles:
        tokens = tokenize(art["text"])
        for t in tokens:
            if t not in lexicon:
                lexicon[t] = current_id
                current_id += 1

    return lexicon

def save_lexicon(lexicon, file_path):
    with open(file_path, 'w', encoding='utf-8') as f:
        for word, idx in lexicon.items():
            f.write(f"{idx}\t{word}\n")


if __name__ == "__main__":
    # 1. Load dataset
    dataset_file = "../data/arxiv-metadata-oai-snapshot.json"
    articles = load_arxiv_dataset(dataset_file, limit=50000)  # adjust as needed

    # 2. Build lexicon
    lexicon = build_lexicon(articles)
    print(f"Total unique words: {len(lexicon)}")

    # 3. Save lexicon
    save_lexicon(lexicon, "../indexes/lexicon.txt")
    print("Lexicon saved to indexes/lexicon.txt")


