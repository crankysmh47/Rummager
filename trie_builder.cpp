#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <queue>

using namespace std;

// --- CONFIGURATION ---
const string JOB_ROOT = "C:\\Users\\Hank47\\Sem3\\Rummager\\";
const string LEXICON_FILE = JOB_ROOT + "lexicon.bin";
const string FORWARD_FILE = JOB_ROOT + "forward_index.bin";
const string TRIE_FILE = JOB_ROOT + "trie.bin";

// --- DATA STRUCTURES ---

// 1. Pointer-Based Trie Node (For Construction)
struct TrieNode {
    char key;
    uint32_t frequency = 0;
    bool isEnd = false;
    vector<TrieNode*> children;

    TrieNode(char c) : key(c) {}
    
    // Helper to find child
    TrieNode* getChild(char c) {
        for (auto child : children) {
            if (child->key == c) return child;
        }
        return nullptr;
    }
};

// 2. Flat Trie Node (For Disk/Cache Efficiency)
struct FlatNode {
    char key;
    int32_t frequency; // 0 if not end of word
    int32_t childIndex; // -1 if no children
    int32_t siblingIndex; // -1 if no next sibling
    bool isEnd; // redundant if freq > 0 used as flag, but keeping for clarity
};

// --- GLOBALS ---
vector<string> idToWord;
unordered_map<string, uint32_t> wordFreqs;

void loadLexicon() {
    cout << "Loading Lexicon..." << endl;
    ifstream file(LEXICON_FILE, ios::binary);
    if (!file) { cerr << "Error opening " << LEXICON_FILE << endl; exit(1); }

    uint32_t totalWords;
    file.read((char*)&totalWords, sizeof(totalWords));
    
    idToWord.resize(totalWords);

    for (uint32_t i = 0; i < totalWords; i++) {
        uint32_t len;
        file.read((char*)&len, sizeof(len));
        string word(len, ' ');
        file.read(&word[0], len);
        idToWord[i] = word;
        // Don't init 0 here, we do it in calc to handle lowercase merging
    }
    cout << "Loaded " << totalWords << " words." << endl;
}

void calculateFrequencies() {
    cout << "Calculating Frequencies from Forward Index..." << endl;
    ifstream file(FORWARD_FILE, ios::binary);
    if (!file) { cerr << "Error opening " << FORWARD_FILE << endl; exit(1); }

    uint32_t docID, totalDocWords, uniqueCount;
    uint32_t wordID, freq;
    long long totalPostings = 0;

    while (file.read((char*)&docID, sizeof(docID))) {
        file.read((char*)&totalDocWords, sizeof(totalDocWords));
        file.read((char*)&uniqueCount, sizeof(uniqueCount));

        for (uint32_t i = 0; i < uniqueCount; ++i) {
            file.read((char*)&wordID, sizeof(wordID));
            file.read((char*)&freq, sizeof(freq));

            if (wordID < idToWord.size()) {
                string word = idToWord[wordID];
                // Normalize to lowercase
                transform(word.begin(), word.end(), word.begin(), ::tolower);
                wordFreqs[word] += freq;
            }
        }
        totalPostings++;
        if (totalPostings % 1000000 == 0) cout << "Processed " << totalPostings << " postings...\r" << flush;
    }
    cout << "\nFrequency calculation complete." << endl;
}

// Recursive helper to flatten the trie
// Returns the index of the node in the flat array
int32_t flatten(TrieNode* node, vector<FlatNode>& flatTrie) {
    if (!node) return -1;

    int32_t myIndex = (int32_t)flatTrie.size();
    flatTrie.push_back({
        node->key, 
        node->isEnd ? (int32_t)node->frequency : 0, 
        -1, // Child Index placeholder
        -1, // Sibling Index placeholder
        node->isEnd
    });

    // Process Children
    // Sort children by key (or frequency/importance if improved matching needed)
    // Here sorting by key helps predictable navigation
    sort(node->children.begin(), node->children.end(), [](TrieNode* a, TrieNode* b){
        return a->key < b->key;
    });

    int32_t prevChildIndex = -1;
    int32_t firstChildIndex = -1;

    for (auto child : node->children) {
        int32_t childIdx = flatten(child, flatTrie);
        
        if (firstChildIndex == -1) firstChildIndex = childIdx;

        if (prevChildIndex != -1) {
            flatTrie[prevChildIndex].siblingIndex = childIdx;
        }
        prevChildIndex = childIdx;
    }

    flatTrie[myIndex].childIndex = firstChildIndex;
    return myIndex;
}

int main() {
    loadLexicon();
    calculateFrequencies();

    cout << "Building Trie..." << endl;
    cout << "Applying Noise Filter (Freq >= 50)..." << endl;
    
    TrieNode* root = new TrieNode('\0'); // Root dummy
    int wordsInserted = 0;

    for (const auto& pair : wordFreqs) {
        const string& word = pair.first;
        uint32_t freq = pair.second;

        // NOISE FILTER
        if (freq < 50) continue;

        TrieNode* curr = root;
        for (char c : word) {
            TrieNode* next = curr->getChild(c);
            if (!next) {
                next = new TrieNode(c);
                curr->children.push_back(next);
            }
            curr = next;
        }
        curr->isEnd = true;
        curr->frequency = freq;
        wordsInserted++;
    }
    cout << "Inserted " << wordsInserted << " words into Trie." << endl;

    cout << "Flattening Trie..." << endl;
    vector<FlatNode> flatTrie;
    // We skip the dummy root for the flat array usually, or keep it.
    // Let's keep children of root as the top level list.
    // Or simpler: Flatten starting from root, but we only really care about its children.
    // Let's perform a custom flatten for root's children to be the entry points?
    // Actually, usually easier to have a single root node at index 0.
    flatten(root, flatTrie);

    // Save
    cout << "Saving " << flatTrie.size() << " nodes to " << TRIE_FILE << "..." << endl;
    ofstream outFile(TRIE_FILE, ios::binary);
    outFile.write((char*)flatTrie.data(), flatTrie.size() * sizeof(FlatNode));
    outFile.close();

    cout << "Done." << endl;
    return 0;
}