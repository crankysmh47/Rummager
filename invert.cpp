#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm> // Needed for max()

using namespace std;

struct Posting {
    uint32_t docID;
    uint32_t freq;
};

int main() {
    // --- PATHS ---
    const string FORWARD_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\forward_index.bin";
    const string LEXICON_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\lexicon.bin";
    const string OUTPUT_FILE  = "C:\\Users\\Hank47\\Sem3\\Rummager\\inverted_index.bin";
    
    // 1. Get Lexicon Size
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) { cerr << "Error: " << LEXICON_FILE << " missing." << endl; return 1; }
    
    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    lexFile.close();

    cout << "Initializing Indexer for " << totalWords << " words..." << endl;

    // 2. Prepare Memory
    vector<vector<Posting>> invertedIndex(totalWords);
    
    ifstream fwdFile(FORWARD_FILE, ios::binary);
    if (!fwdFile) { cerr << "Error: " << FORWARD_FILE << " missing." << endl; return 1; }

    uint32_t docID, totalDocWords, uniqueCount;
    uint32_t wordID, freq;
    int docsProcessed = 0;

    cout << "Reading Forward Index..." << endl;

    while (fwdFile.read((char*)&docID, sizeof(docID))) {
        fwdFile.read((char*)&totalDocWords, sizeof(totalDocWords));
        fwdFile.read((char*)&uniqueCount, sizeof(uniqueCount));

        for (uint32_t i = 0; i < uniqueCount; ++i) {
            fwdFile.read((char*)&wordID, sizeof(wordID));
            fwdFile.read((char*)&freq, sizeof(freq));

            if (wordID < totalWords) {
                invertedIndex[wordID].push_back({docID, freq});
            }
        }

        docsProcessed++;
        if (docsProcessed % 10000 == 0) cout << "Processed " << docsProcessed << " docs...\r" << flush;
    }
    fwdFile.close();

    // 3. Write INVERTED INDEX to Disk
    cout << "\nWriting Inverted Index..." << endl;
    ofstream outFile(OUTPUT_FILE, ios::binary);
    
    // Header
    outFile.write((char*)&totalWords, sizeof(totalWords));

    for (uint32_t i = 0; i < totalWords; ++i) {
        uint32_t listSize = (uint32_t)invertedIndex[i].size();
        outFile.write((char*)&listSize, sizeof(listSize));
        if (listSize > 0) {
            outFile.write((char*)invertedIndex[i].data(), listSize * sizeof(Posting));
        }
    }
    outFile.close();

    cout << "Success! Inverted Index created." << endl;
    return 0;
}

/*
    ========================================================================================
    EDUCATIONAL SUMMARY: INVERTED INDEX CONSTRUCTION
    ========================================================================================
    
    1. THE GOAL
       - Convert "Forward Index" (Doc -> Words) into "Inverted Index" (Word -> Docs).
       - This is crucial for search: "Find all docs containing Word X".
    
    2. THE ALGORITHM
       - In this project, we use "In-Memory Inversion" because our lexicon fits in RAM.
       - We create an array of lists: `vector<vector<Posting>> index(TotalWords)`.
       - We read the Forward Index stream (DocID, List of Words).
       - For each WordID in the doc, we append `{DocID, Freq}` to `index[WordID]`.
    
    3. SCALABILITY NOTE
       - If the index were too large for RAM (e.g., Google scale), we would use:
         "External Sort-Based Inversion" (BSBI or SPIMI).
         - Write (WordID, DocID) pairs to disk.
         - Sort them by WordID using Merge Sort.
         - Compress into Posting Lists.
*/