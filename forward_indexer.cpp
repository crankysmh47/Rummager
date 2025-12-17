#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>
#include "common.h" // <--- INCLUDES STOPWORDS & TOKENIZER

using namespace std;

// This Lexicon class is for LOADING (Reading), not building
class LexiconLoader {
private:
    unordered_map<string, int> wordToID;

public:
    bool loadBinary(const string& filename) {
        ifstream inFile(filename, ios::binary);
        if (!inFile) return false;

        uint32_t size;
        inFile.read((char*)&size, sizeof(size));

        for (uint32_t i = 0; i < size; ++i) {
            uint32_t len;
            inFile.read((char*)&len, sizeof(len));
            string word(len, ' ');
            inFile.read(&word[0], len);
            wordToID[word] = i; 
        }
        return true;
    }

    int getID(const string& word) const {
        auto it = wordToID.find(word);
        return (it != wordToID.end()) ? it->second : -1;
    }
    
    size_t size() const { return wordToID.size(); }
};

int main() {
    LexiconLoader lexicon;
    if (!lexicon.loadBinary("C:\\Users\\Hank47\\Sem3\\Rummager\\lexicon.bin")) {
        cerr << "Error: lexicon.bin missing. Run builder first." << endl;
        return 1;
    }

    ifstream infile("C:\\Users\\Hank47\\Sem3\\Rummager\\clean_dataset.txt");
    if (!infile) {
        cerr << "Error: clean_dataset.txt not found." << endl;
        return 1;
    }

    // LOAD ID MAP (String -> Int)
    unordered_map<string, uint32_t> idMap;
    ifstream mapFile("C:\\Users\\Hank47\\Sem3\\Rummager\\id_map.txt");
    if (!mapFile) {
        cerr << "Error: id_map.txt not found. Run graph_parser.py first." << endl;
        return 1;
    }
    string strID;
    uint32_t intID;
    uint32_t maxID = 0;
    while (mapFile >> strID >> intID) {
        idMap[strID] = intID;
        if (intID > maxID) maxID = intID;
    }
    mapFile.close();
    cout << "Loaded ID Map with " << idMap.size() << " entries. Max ID: " << maxID << endl;

    ofstream outfile("C:\\Users\\Hank47\\Sem3\\Rummager\\forward_index.bin", ios::binary);
    ofstream lenFile("C:\\Users\\Hank47\\Sem3\\Rummager\\doc_lengths.bin", ios::binary);
    
    // We need random access to lengths now, so use a vector
    vector<uint32_t> lengthsBuffer(maxID + 1, 0);

    string line;
    uint32_t docsProcessed = 0;

    cout << "Starting Forward Indexing..." << endl;

    while (getline(infile, line)) {
        // input format: DocIDVal <tab> Content...
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) continue;

        string docIDStr = line.substr(0, tabPos);
        string content = line.substr(tabPos + 1);

        // Resolve ID
        if (idMap.find(docIDStr) == idMap.end()) {
            // If not found in map (maybe preprocessed file has more docs than graph?), skip or warn
            // For now, skip to match graph consistency
            continue;
        }
        uint32_t uDocID = idMap[docIDStr];

        vector<string> tokens = Tokenize::tokenize(content);

        // Map: WordID -> Frequency
        map<int, int> docWordFreq;
        int totalWordsInDoc = 0;

        for (const string& token : tokens) {
            int id = lexicon.getID(token);
            if (id != -1) {
                docWordFreq[id]++;
                totalWordsInDoc++;
            }
        }

        // Write to Forward Index
        uint32_t uTotal = (uint32_t)totalWordsInDoc;
        uint32_t uUnique = (uint32_t)docWordFreq.size();

        outfile.write((char*)&uDocID, sizeof(uDocID));
        outfile.write((char*)&uTotal, sizeof(uTotal));
        outfile.write((char*)&uUnique, sizeof(uUnique));

        for (const auto& pair : docWordFreq) {
            uint32_t uWordID = (uint32_t)pair.first;
            uint32_t uFreq = (uint32_t)pair.second;
            outfile.write((char*)&uWordID, sizeof(uWordID));
            outfile.write((char*)&uFreq, sizeof(uFreq));
        }

        // Store Length
        if (uDocID < lengthsBuffer.size()) {
            lengthsBuffer[uDocID] = uTotal;
        }

        docsProcessed++;
        if (docsProcessed % 1000 == 0) cout << "Indexed " << docsProcessed << " docs...\r" << flush;
    }

    // Write Lengths File
    uint32_t totalDocs = lengthsBuffer.size();
    lenFile.write((char*)&totalDocs, sizeof(totalDocs)); // Header
    lenFile.write((char*)lengthsBuffer.data(), totalDocs * sizeof(uint32_t)); // Data

    infile.close();
    outfile.close();
    lenFile.close();

    cout << "\nIndex Complete! Processed " << docsProcessed << " documents. Mapped to " << totalDocs << " IDs." << endl;
    return 0;
}

/*
    ========================================================================================
    EDUCATIONAL SUMMARY: FORWARD INDEXING
    ========================================================================================
    
    1. THE GOAL
       - Convert raw text documents into a structured intermediate format.
       - Format: DocID -> [WordID1, WordID2, ...]
    
    2. KEY METRICS EXTRACTED
       - Term Frequency (TF): How many times a word appears in THIS doc. Sourced for BM25.
       - Document Length (DL): Total words in the doc. Crucial for BM25 Normalization.
    
    3. DATA STRUCTURE
       - We use a Hash Map (`map<int, int> docWordFreq`) per document to count frequencies.
       - We write to disk immediately to keep RAM usage low (Stream Processing).
    
    4. WHY "FORWARD" FIRST?
       - We cannot build the Inverted Index directly because we process docs one by one.
       - We first build the Forward Index (Doc-centric), then "Invert" it (Word-centric).
*/