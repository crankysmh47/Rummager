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
    if (!lexicon.loadBinary("lexicon.bin")) {
        cerr << "Error: lexicon.bin missing. Run builder first." << endl;
        return 1;
    }

    ifstream infile("clean_dataset.txt"); 
    ofstream outfile("forward_index.bin", ios::binary);
    ofstream mapfile("docid_map.txt");

    if (!infile || !outfile || !mapfile) {
        cerr << "File error." << endl; 
        return 1;
    }

    string line;
    uint32_t internalDocID = 0;
    map<int, int> docWordFreq; 

    cout << "Generating Forward Index..." << endl;

    while (getline(infile, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) continue;

        string originalDocID = line.substr(0, tabPos);
        string content = line.substr(tabPos + 1);

        mapfile << internalDocID << " " << originalDocID << "\n";

        // USES SHARED TOKENIZER (Guaranteed same logic as builder)
        vector<string> tokens = Tokenize::tokenize(content);
        
        docWordFreq.clear();
        int totalWordsInDoc = 0;

        for (const string& token : tokens) {
            int id = lexicon.getID(token);
            if (id != -1) {
                docWordFreq[id]++;
                totalWordsInDoc++;
            }
        }

        uint32_t uDocID = internalDocID;
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

        internalDocID++;
        if (internalDocID % 10000 == 0) cout << "Indexed " << internalDocID << "...\r" << flush;
    }

    cout << "\nDone!" << endl;
    return 0;
}