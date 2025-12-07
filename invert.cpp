#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

// We need a simple struct to hold both pieces of data
struct Posting {
    uint32_t docID;
    uint32_t freq;
};

int main() {
    const string FORWARD_FILE = "forward_index.bin";
    const string LEXICON_FILE = "lexicon.bin";
    const string OUTPUT_FILE = "inverted_index.bin";

    // 1. Get Lexicon Size
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) {
        cerr << "Error: lexicon.bin not found." << endl;
        return 1;
    }
    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    lexFile.close();

    cout << "Initializing Inverted Index for " << totalWords << " unique words..." << endl;

    // 2. Create Memory Structure 
    // NOW STORING POSTINGS (DocID + Freq), not just DocIDs
    vector<vector<Posting>> invertedIndex(totalWords);

    ifstream fwdFile(FORWARD_FILE, ios::binary);
    if (!fwdFile) return 1;

    uint32_t docID, totalDocWords, uniqueCount;
    uint32_t wordID, freq;
    int docsProcessed = 0;

    cout << "Inverting data..." << endl;

    while (fwdFile.read((char*)&docID, sizeof(docID))) {
        fwdFile.read((char*)&totalDocWords, sizeof(totalDocWords));
        fwdFile.read((char*)&uniqueCount, sizeof(uniqueCount));

        for (uint32_t i = 0; i < uniqueCount; ++i) {
            fwdFile.read((char*)&wordID, sizeof(wordID));
            fwdFile.read((char*)&freq, sizeof(freq));

            if (wordID < totalWords) {
                // STORE FREQUENCY HERE
                invertedIndex[wordID].push_back({docID, freq});
            }
        }

        docsProcessed++;
        if (docsProcessed % 10000 == 0) cout << "Inverted " << docsProcessed << " docs...\r" << flush;
    }
    fwdFile.close();

    // 3. Write Inverted Index to Disk
    cout << "\nWriting Inverted Index with Frequencies..." << endl;
    
    ofstream outFile(OUTPUT_FILE, ios::binary);
    
    // Header
    outFile.write((char*)&totalWords, sizeof(totalWords));

    for (uint32_t i = 0; i < totalWords; ++i) {
        uint32_t listSize = (uint32_t)invertedIndex[i].size();
        outFile.write((char*)&listSize, sizeof(listSize));
        
        // Write the list of Postings (DocID + Freq)
        if (listSize > 0) {
            // We can write the vector of structs directly as a block of memory
            // Format on disk: [DocID][Freq][DocID][Freq]...
            outFile.write((char*)invertedIndex[i].data(), listSize * sizeof(Posting));
        }
    }
    
    outFile.close();
    cout << "Success! Inverted Index created (Rank-Ready)." << endl;

    return 0;
}