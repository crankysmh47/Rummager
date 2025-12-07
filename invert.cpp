#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>

using namespace std;

int main() {
    const string FORWARD_FILE = "forward_index.bin";
    const string LEXICON_FILE = "lexicon.bin";
    const string OUTPUT_FILE = "inverted_index.bin";

    // 1. Get Lexicon Size (to resize vector)
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) {
        cerr << "Error: lexicon.bin not found." << endl;
        return 1;
    }
    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    lexFile.close();

    cout << "Initializing Inverted Index for " << totalWords << " unique words..." << endl;

    // 2. Create Memory Structure (WordID -> List of DocIDs)
    // We use vector of vectors. This fits in RAM for ArXiv (approx 800MB - 1.5GB RAM).
    vector<vector<uint32_t>> invertedIndex(totalWords);

    // 3. Read Forward Index and Fill Inverted Index
    ifstream fwdFile(FORWARD_FILE, ios::binary);
    if (!fwdFile) {
        cerr << "Error: forward_index.bin not found." << endl;
        return 1;
    }

    uint32_t docID, totalDocWords, uniqueCount;
    uint32_t wordID, freq;
    int docsProcessed = 0;

    cout << "Inverting data..." << endl;

    // Read loop must match exactly how you wrote it in main.cpp
    while (fwdFile.read((char*)&docID, sizeof(docID))) {
        fwdFile.read((char*)&totalDocWords, sizeof(totalDocWords));
        fwdFile.read((char*)&uniqueCount, sizeof(uniqueCount));

        for (uint32_t i = 0; i < uniqueCount; ++i) {
            fwdFile.read((char*)&wordID, sizeof(wordID));
            fwdFile.read((char*)&freq, sizeof(freq));

            // THE PIVOT: Add this DocID to the Word's list
            if (wordID < totalWords) {
                invertedIndex[wordID].push_back(docID);
            }
        }

        docsProcessed++;
        if (docsProcessed % 10000 == 0) cout << "Inverted " << docsProcessed << " docs...\r" << flush;
    }
    fwdFile.close();

    // 4. Write Inverted Index to Disk
    // Format: [WordID] -> [Count] -> [DocID1, DocID2, ...]
    cout << "\nWriting Inverted Index to " << OUTPUT_FILE << "..." << endl;
    
    ofstream outFile(OUTPUT_FILE, ios::binary);
    
    // Write total number of words first (header)
    outFile.write((char*)&totalWords, sizeof(totalWords));

    for (uint32_t i = 0; i < totalWords; ++i) {
        uint32_t listSize = (uint32_t)invertedIndex[i].size();
        outFile.write((char*)&listSize, sizeof(listSize));
        
        // Write the list of DocIDs
        if (listSize > 0) {
            outFile.write((char*)invertedIndex[i].data(), listSize * sizeof(uint32_t));
        }
    }
    
    outFile.close();
    cout << "Success! Inverted Index created." << endl;

    return 0;
}