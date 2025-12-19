#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace std;

// --- CONFIGURATION ---
const string DATASET_FILE = "clean_dataset.txt";
const string OUTPUT_FILE = "cortex.json";
const int WINDOW_SIZE = 5;
const int MIN_WORD_FREQ = 50;   // Word must appear this often to be learned
const int MAX_VOCAB_SIZE = 50000; // Keep top N distinct words to save RAM
const int TOP_K_ASSOCIATIONS = 20; // Save top 20 synonyms per word

// --- HELPERS ---

// Stopwords (Basic list to reduce noise)
unordered_set<string> STOPWORDS = {
    "the", "of", "and", "in", "to", "a", "is", "for", "with", "that", "on", "as", "are", "by", "at", "an", "be", "this", "which", "from", "or", "it", "can", "have", "has", "but", "not", "we", "image", "data", "using"
};

// Clean and tokenize
string cleanWord(const string& w) {
    string out;
    for (char c : w) {
        if (isalnum(c)) {
            out += tolower(c);
        }
    }
    return out;
}

// Global Structures
unordered_map<string, int> vocabFreq;
unordered_map<string, unordered_map<string, int>> cooc;

int main() {
    auto start = chrono::high_resolution_clock::now();
    cout << "--- Aether C++ Semantic Trainer ---" << endl;
    cout << "Dataset: " << DATASET_FILE << endl;
    cout << "Window: " << WINDOW_SIZE << endl;

    // PHASE 1: BUILD VOCABULARY (Single Pass or On-the-fly?)
    // Due to 3.4GB size, let's do 2 passes.
    // Pass 1: Count Frequencies (to identify common words and prune rare ones for RAM safety)
    
    cout << "\n[Phase 1] Counting Vocab Frequency..." << endl;
    ifstream file(DATASET_FILE);
    if (!file) {
        cerr << "Error: Could not open " << DATASET_FILE << endl;
        return 1;
    }

    string line;
    long long lineCount = 0;
    while (getline(file, line)) {
        if (++lineCount % 100000 == 0) cout << "Scanned " << lineCount << " lines..." << "\r";
        
        // Tab separated? Or just text. The format is ID <col> Text
        // We just grab all words.
        stringstream ss(line);
        string word;
        while (ss >> word) {
            word = cleanWord(word);
            if (word.length() > 2 && STOPWORDS.find(word) == STOPWORDS.end()) {
                 vocabFreq[word]++;
            }
        }
    }
    file.close();
    
    // Pruning Vocab
    cout << "\nTotal Unique Words: " << vocabFreq.size() << endl;
    vector<pair<int, string>> sortedVocab;
    for (auto& kv : vocabFreq) {
        if (kv.second >= MIN_WORD_FREQ) {
            sortedVocab.push_back({kv.second, kv.first});
        }
    }
    sort(sortedVocab.rbegin(), sortedVocab.rend()); // Sort Descending
    
    if (sortedVocab.size() > MAX_VOCAB_SIZE) {
        sortedVocab.resize(MAX_VOCAB_SIZE);
    }
    
    // Valid Set for O(1) lookup
    unordered_set<string> validWords;
    for (auto& p : sortedVocab) validWords.insert(p.second);
    
    cout << "Pruned Vocab Size: " << validWords.size() << endl;
    
    // PHASE 2: CO-OCCURRENCE
    cout << "\n[Phase 2] Building Co-occurrence Matrix..." << endl;
    file.open(DATASET_FILE);
    lineCount = 0;
    
    while (getline(file, line)) {
        if (++lineCount % 50000 == 0) cout << "Processed " << lineCount << " lines..." << "\r";

        // Extract valid tokens from line
        vector<string> tokens;
        stringstream ss(line);
        string word;
        while (ss >> word) {
            word = cleanWord(word);
            if (validWords.count(word)) {
                tokens.push_back(word);
            }
        }

        // Sliding Window
        for (int i = 0; i < (int)tokens.size(); i++) {
            string target = tokens[i];
            
            // Look back and forward
            int start = max(0, i - WINDOW_SIZE);
            int end = min((int)tokens.size() - 1, i + WINDOW_SIZE);
            
            for (int j = start; j <= end; j++) {
                if (i == j) continue;
                string context = tokens[j];
                cooc[target][context]++;
            }
        }
    }
    file.close();

    // PHASE 3: EXPORT JSON
    cout << "\n[Phase 3] Exporting to " << OUTPUT_FILE << "..." << endl;
    ofstream out(OUTPUT_FILE);
    out << "{" << endl;
    
    int wordIdx = 0;
    int totalExport = validWords.size();
    
    bool firstWord = true;
    for (const string& w : validWords) {
        if (!firstWord) out << "," << endl;
        firstWord = false;
        
        out << "  \"" << w << "\": [";
        
        // Sort associations
        vector<pair<int, string>> assocs;
        for (auto& kv : cooc[w]) {
            assocs.push_back({kv.second, kv.first});
        }
        sort(assocs.rbegin(), assocs.rend());
        
        if (assocs.size() > TOP_K_ASSOCIATIONS) assocs.resize(TOP_K_ASSOCIATIONS);
        
        for (int i = 0; i < assocs.size(); i++) {
            if (i > 0) out << ", ";
            out << "\"" << assocs[i].second << "\"";
        }
        out << "]";
    }
    out << "\n}" << endl;
    out.close();

    cout << "\nTraining Complete!" << endl;
    return 0;
}
