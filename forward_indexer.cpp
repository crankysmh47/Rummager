#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>
#include "common.h" // Includes STOPWORDS & TOKENIZER

using namespace std;

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
    ofstream lenFile("doc_lengths.bin", ios::binary);

    if (!infile.is_open() || !outfile.is_open() || !lenFile.is_open()) {
        cerr << "Error opening file(s)!" << endl;
        return 1;
    }

    // Placeholder for total docs; will rewrite later
    uint32_t totalDocs = 0;
    lenFile.write((char*)&totalDocs, sizeof(totalDocs));

    string line;
    uint32_t internalDocID = 0;

    while (getline(infile, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) continue;

        string content = line.substr(tabPos + 1);
        vector<string> tokens = Tokenize::tokenize(content);

        map<int, int> docWordFreq;
        uint32_t totalWordsInDoc = 0;

        for (const string& token : tokens) {
            int wordID = lexicon.getID(token);
            if (wordID != -1) {
                docWordFreq[wordID]++;
                totalWordsInDoc++;
            }
        }

        uint32_t uDocID = internalDocID;
        uint32_t uTotal = totalWordsInDoc;
        uint32_t uUnique = docWordFreq.size();

        // Write forward index
        outfile.write((char*)&uDocID, sizeof(uDocID));
        outfile.write((char*)&uTotal, sizeof(uTotal));
        outfile.write((char*)&uUnique, sizeof(uUnique));

        for (const auto& pair : docWordFreq) {
            uint32_t uWordID = pair.first;
            uint32_t uFreq = pair.second;
            outfile.write((char*)&uWordID, sizeof(uWordID));
            outfile.write((char*)&uFreq, sizeof(uFreq));
        }

        // Write doc length
        lenFile.write((char*)&uTotal, sizeof(uTotal));

        internalDocID++;
        if (internalDocID % 10000 == 0)
            cout << "Indexed " << internalDocID << " documents...\r" << flush;
    }

    // Rewrite totalDocs at start of lengths file
    lenFile.seekp(0);
    lenFile.write((char*)&internalDocID, sizeof(internalDocID));

    infile.close();
    outfile.close();
    lenFile.close();

    cout << "\nDone! Total documents indexed: " << internalDocID << endl;
    return 0;
}
