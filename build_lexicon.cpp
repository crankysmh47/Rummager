#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "common.h" // <--- INCLUDES STOPWORDS & TOKENIZER

using namespace std;

class LexiconBuilder {
private:
    unordered_map<string, int> wordToID;
    vector<string> idToWord;

public:
    int getID(const string& word) {
        if (wordToID.find(word) != wordToID.end()) {
            return wordToID[word];
        }
        int newID = idToWord.size();
        wordToID[word] = newID;
        idToWord.push_back(word);
        return newID;
    }

    void saveBinary(const string& filename) {
        ofstream outFile(filename, ios::binary);
        if (!outFile) {
            cerr << "Error writing " << filename << endl;
            return;
        }
        uint32_t size = (uint32_t)idToWord.size();
        outFile.write((char*)&size, sizeof(size));

        for (const string& word : idToWord) {
            uint32_t len = (uint32_t)word.size();
            outFile.write((char*)&len, sizeof(len));
            outFile.write(word.c_str(), len);
        }
        outFile.close();
    }
    
    // Helper to print debug info
    int getTotalWords() { return idToWord.size(); }
};

int main() {
    LexiconBuilder lexicon;
    ifstream file("clean_dataset.txt");
    
    if (!file.is_open()) {
        cerr << "Error: Run preprocess.py first!" << endl;
        return 1;
    }

    string line;
    int count = 0;
    cout << "Building Lexicon..." << endl;

    while (getline(file, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) continue;

        string content = line.substr(tabPos + 1);
        
        // USES SHARED TOKENIZER
        vector<string> tokens = Tokenize::tokenize(content);

        for (const string& token : tokens) {
            lexicon.getID(token);
        }

        count++;
        if (count % 10000 == 0) cout << "Processed " << count << " lines...\r" << flush;
    }

    cout << "\nSaving lexicon.bin..." << endl;
    lexicon.saveBinary("lexicon.bin");
    cout << "Done. Total words: " << lexicon.getTotalWords() << endl;

    return 0;
}