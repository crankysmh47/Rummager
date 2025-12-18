#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <sys/stat.h>

using namespace std;

// --- CONFIGURATION ---
const string INVERTED_INDEX_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\inverted_index.bin";
string BARREL_DIR = "C:\\Users\\Hank47\\Sem3\\Rummager\\barrels\\"; // Not const anymore
const string LEXICON_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\lexicon.bin";

// MUST match searchengine.cpp
const uint32_t WORDS_PER_BARREL = 50000; 

struct Posting {
    uint32_t docID;
    uint32_t freq;
};

// Helper: Ensure directory exists
void createDir(const string& path) {
    #ifdef _WIN32
        string cmd = "mkdir \"" + path + "\" 2> NUL";
        system(cmd.c_str());
    #else
        system(("mkdir -p " + path).c_str());
    #endif
}

void writeBarrel(int barrelID, const vector<vector<Posting>>& barrelData) {
    string filename = BARREL_DIR + "barrel_" + to_string(barrelID) + ".bin";
    ofstream outFile(filename, ios::binary);
    
    if (!outFile) {
        cerr << "Error: Could not create " << filename << endl;
        return;
    }

    // 1. Prepare Offset Table
    // The table has one entry (long long = 8 bytes) per potential word in this barrel.
    // If a word has no postings, the offset is 0.
    vector<long long> offsets(WORDS_PER_BARREL, 0);

    // Calculate where the ACTUAL data starts.
    // Data starts strictly after the Offset Table.
    long long currentFileOffset = WORDS_PER_BARREL * sizeof(long long);

    // 2. Calculate Offsets
    for (uint32_t i = 0; i < WORDS_PER_BARREL; ++i) {
        if (i < barrelData.size() && !barrelData[i].empty()) {
            offsets[i] = currentFileOffset;
            
            // Size of this posting list on disk:
            // [ListSize (4 bytes)] + [Posting1] + [Posting2] ...
            long long listByteSize = sizeof(uint32_t) + (barrelData[i].size() * sizeof(Posting));
            
            currentFileOffset += listByteSize;
        }
    }

    // 3. Write Offset Table
    cout << "  Writing Barrel " << barrelID << " Header (Offsets)..." << endl;
    outFile.write((char*)offsets.data(), offsets.size() * sizeof(long long));

    // 4. Write Data
    cout << "  Writing Posting Lists..." << endl;
    for (uint32_t i = 0; i < WORDS_PER_BARREL; ++i) {
        if (i < barrelData.size() && !barrelData[i].empty()) {
            uint32_t listSize = (uint32_t)barrelData[i].size();
            outFile.write((char*)&listSize, sizeof(listSize));
            outFile.write((char*)barrelData[i].data(), listSize * sizeof(Posting));
        }
    }

    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        BARREL_DIR = argv[1];
        // Ensure trailing slash
        if (BARREL_DIR.back() != '\\' && BARREL_DIR.back() != '/') {
            BARREL_DIR += "\\";
        }
    }
    cout << "Output Directory: " << BARREL_DIR << endl;
    createDir(BARREL_DIR);

    // 1. Get Lexicon Size (to know total words)
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) {
        cerr << "Error: lexicon.bin not found. Run build_lexicon first." << endl;
        return 1;
    }
    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    lexFile.close();

    cout << "Total Words: " << totalWords << ". Batch Size: " << WORDS_PER_BARREL << endl;

    // 2. Open Inverted Index
    ifstream invFile(INVERTED_INDEX_FILE, ios::binary);
    if (!invFile) {
        cerr << "Error: inverted_index.bin not found. Run invert first." << endl;
        return 1;
    }

    // Skip Header (Total Words)
    uint32_t checkTotal;
    invFile.read((char*)&checkTotal, sizeof(checkTotal));

    // 3. Sequential Processing (Barrel by Barrel)
    int currentBarrelID = 0;
    
    // We process WORDS sequentially. 
    // Barrel 0: Words 0 to 49,999
    // Barrel 1: Words 50,000 to 99,999
    // ...
    
    while (currentBarrelID * WORDS_PER_BARREL < totalWords) {
        uint32_t startWord = currentBarrelID * WORDS_PER_BARREL;
        uint32_t endWord = startWord + WORDS_PER_BARREL; // Exclusive
        if (endWord > totalWords) endWord = totalWords;

        cout << "Building Barrel " << currentBarrelID << " (Words " << startWord << "-" << (endWord-1) << ")..." << endl;

        // In-Memory Storage for THIS Barrel only
        vector<vector<Posting>> barrelData(WORDS_PER_BARREL);

        // Read from Inverted Index (Assumption: Inverted Index is sorted by WordID)
        // Since inverted_index.bin IS sorted by WordID 0...N (created by invert.cpp loop),
        // we can just read sequentially!
        
        for (uint32_t w = startWord; w < endWord; ++w) {
            uint32_t listSize;
            invFile.read((char*)&listSize, sizeof(listSize));
            
            if (listSize > 0) {
                vector<Posting> postings(listSize);
                invFile.read((char*)postings.data(), listSize * sizeof(Posting));
                
                // Validate
                if (invFile.gcount() != listSize * sizeof(Posting)) {
                    cerr << "Error reading postings for word " << w << endl;
                    break;
                }

                // Store in local barrel buffer (0-indexed relative to barrel)
                barrelData[w - startWord] = postings;
            }
        }

        // Write to Disk
        writeBarrel(currentBarrelID, barrelData);

        currentBarrelID++;
    }

    invFile.close();
    cout << "Success! Created " << currentBarrelID << " barrels." << endl;
    return 0;
}

/*
    ========================================================================================
    EDUCATIONAL SUMMARY: BARRELS & INDEX SHARDING
    ========================================================================================
    
    1. THE PROBLEM: RAM LIMITS
       - A full inverted index for the web is TBs in size.
       - We cannot load vector<vector<Posting>> for 10M words into RAM.
    
    2. THE SOLUTION: SHARDING (BARRELS)
       - We split the index into smaller chunks called "Barrels".
       - Strategy: "Term Partitioning"
         - Barrel 0: Words 0 - 49,999
         - Barrel 1: Words 50,000 - 99,999
       - When searching for a word, we calculate BarrelID = WordID / 50000.
       - We only open that specific file.
    
    3. DATA STRUCTURE: OFFSET TABLE (O(1) LOOKUP)
       - Inside a barrel, we don't want to scan to find a word's list.
       - We place a "Header" at the start of the file: an array of offsets.
       - Logic:
         - Need word 105 (Local ID = 5)?
         - Seek to byte 5 * 8 (8 bytes per long long).
         - Read the Offset value (e.g., 2048).
         - Seek to byte 2048.
         - Read the data.
       - This guarantees single-seek retrieval time, critical for speed.
*/