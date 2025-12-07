#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <sys/stat.h>

using namespace std;

// 1. Define the Posting Structure (Rank-Ready)
struct Posting {
    uint32_t docID;
    uint32_t freq;
};

// 2. Configuration
const string FORWARD_FILE = "forward_index.bin";
const string LEXICON_FILE = "lexicon.bin";
const string BARREL_DIR = "barrels/"; // Ensure this folder exists
const int DOCS_PER_BARREL = 10000;    // Adjust based on your RAM

// Helper to write a barrel to disk
void saveBarrel(int barrelID, const vector<vector<Posting>>& index, uint32_t totalWords) {
    string filename = BARREL_DIR + "barrel_" + to_string(barrelID) + ".bin";
    ofstream outFile(filename, ios::binary);
    
    if (!outFile) {
        cerr << "Error: Could not create " << filename << endl;
        return;
    }

    cout << "Writing Barrel " << barrelID << " to disk..." << endl;

    for (uint32_t wordID = 0; wordID < totalWords; ++wordID) {
        uint32_t listSize = (uint32_t)index[wordID].size();
        
        // Optimization: Only write words that actually appeared in this batch
        if (listSize > 0) {
            // Format: [WordID] [ListSize] [Posting1] [Posting2]...
            outFile.write((char*)&wordID, sizeof(wordID));
            outFile.write((char*)&listSize, sizeof(listSize));
            outFile.write((char*)index[wordID].data(), listSize * sizeof(Posting));
        }
    }
    
    outFile.close();
}

int main() {
    // Create directory for barrels (Linux/Mac command, use _mkdir for Windows if needed)
    #ifdef _WIN32
        system("mkdir barrels");
    #else
        system("mkdir -p barrels");
    #endif

    // 1. Load Lexicon Info
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) {
        cerr << "Error: lexicon.bin not found." << endl;
        return 1;
    }
    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    lexFile.close();

    // 2. Prepare Memory
    vector<vector<Posting>> currentBarrel(totalWords);
    
    ifstream fwdFile(FORWARD_FILE, ios::binary);
    if (!fwdFile) {
        cerr << "Error: forward_index.bin not found." << endl;
        return 1;
    }

    uint32_t docID, totalDocWords, uniqueCount;
    uint32_t wordID, freq;
    int docsInCurrentBarrel = 0;
    int barrelCount = 0;

    cout << "Starting Barrel Creation. Batch size: " << DOCS_PER_BARREL << " docs." << endl;

    // 3. Main Loop
    while (fwdFile.read((char*)&docID, sizeof(docID))) {
        fwdFile.read((char*)&totalDocWords, sizeof(totalDocWords));
        fwdFile.read((char*)&uniqueCount, sizeof(uniqueCount));

        for (uint32_t i = 0; i < uniqueCount; ++i) {
            fwdFile.read((char*)&wordID, sizeof(wordID));
            fwdFile.read((char*)&freq, sizeof(freq));

            if (wordID < totalWords) {
                currentBarrel[wordID].push_back({docID, freq});
            }
        }

        docsInCurrentBarrel++;

        // 4. Check Limit -> Dump Barrel
        if (docsInCurrentBarrel >= DOCS_PER_BARREL) {
            saveBarrel(barrelCount, currentBarrel, totalWords);
            
            // Reset for next batch
            barrelCount++;
            docsInCurrentBarrel = 0;
            
            // Clear vectors but keep structure to avoid re-allocation
            for (auto& list : currentBarrel) {
                list.clear();
            }
        }
    }

    // 5. Dump remaining data (the last partial barrel)
    if (docsInCurrentBarrel > 0) {
        saveBarrel(barrelCount, currentBarrel, totalWords);
    }

    fwdFile.close();
    cout << "Done! Created " << (barrelCount + 1) << " barrels in '" << BARREL_DIR << "'." << endl;

    return 0;
}