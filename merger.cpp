#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

using namespace std;

// The same struct as before
struct Posting {
    uint32_t docID;
    uint32_t freq;
};

// This struct helps us manage multiple open files at once
struct BarrelStream {
    ifstream file;
    uint32_t currentWordID;
    uint32_t currentCount;
    bool finished;
    int id;

    // Try to read the next header (WordID + Count) from this barrel
    void advance() {
        if (file.read((char*)&currentWordID, sizeof(currentWordID))) {
            file.read((char*)&currentCount, sizeof(currentCount));
            finished = false;
        } else {
            finished = true;
            currentWordID = -1; // Max value, so it won't match small IDs
        }
    }
};

int main() {
    const string LEXICON_FILE = "lexicon.bin";
    const string BARREL_DIR = "barrels/";
    const string FINAL_INDEX = "final_index.bin";
    const string INDEX_TABLE = "index_table.bin";

    // 1. Get Lexicon Size
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) {
        cerr << "Error: lexicon.bin not found." << endl;
        return 1;
    }
    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    lexFile.close();

    cout << "Merging barrels for " << totalWords << " unique words..." << endl;

    // 2. Open All Barrels
    vector<BarrelStream> barrels;
    int barrelID = 0;
    
    while (true) {
        string fname = BARREL_DIR + "barrel_" + to_string(barrelID) + ".bin";
        ifstream test(fname, ios::binary);
        if (!test) break; // Stop when we run out of barrel files
        
        // Use 'new' or move logic, but for simplicity we push back then open
        BarrelStream bs;
        bs.id = barrelID;
        bs.file.open(fname, ios::binary);
        bs.finished = false;
        
        // Read the first header of this barrel to prime it
        bs.advance();
        
        barrels.push_back(move(bs));
        barrelID++;
    }

    if (barrels.empty()) {
        cerr << "Error: No barrels found in " << BARREL_DIR << endl;
        return 1;
    }
    cout << "Found " << barrels.size() << " barrels. Starting merge..." << endl;

    // 3. Prepare Outputs
    ofstream finalFile(FINAL_INDEX, ios::binary);
    ofstream tableFile(INDEX_TABLE, ios::binary); // Stores (Offset, Count) for each word

    // 4. K-Way Merge
    // We iterate through every possible WordID from 0 to TotalWords
    for (uint32_t targetWordID = 0; targetWordID < totalWords; ++targetWordID) {
        
        vector<Posting> mergedList;
        
        // Check every barrel to see if it has data for 'targetWordID'
        for (auto &barrel : barrels) {
            if (!barrel.finished && barrel.currentWordID == targetWordID) {
                // This barrel has data for the word we are currently merging!
                
                // Read the postings
                vector<Posting> temp(barrel.currentCount);
                barrel.file.read((char*)temp.data(), barrel.currentCount * sizeof(Posting));
                
                // Add to our main list
                mergedList.insert(mergedList.end(), temp.begin(), temp.end());
                
                // Move this barrel to its next word
                barrel.advance();
            }
        }

        // 5. Write to Final Index and Index Table
        
        // Save current position (Offset)
        uint64_t offset = finalFile.tellp(); // Using 64-bit for large files
        uint32_t totalFreq = (uint32_t)mergedList.size();

        // Write entry to Index Table: [Offset (8 bytes)] [Count (4 bytes)]
        tableFile.write((char*)&offset, sizeof(offset));
        tableFile.write((char*)&totalFreq, sizeof(totalFreq));

        // Write actual data to Final Index
        if (totalFreq > 0) {
            finalFile.write((char*)mergedList.data(), totalFreq * sizeof(Posting));
        }

        if (targetWordID % 1000 == 0) 
            cout << "Merged word " << targetWordID << " / " << totalWords << "\r" << flush;
    }

    // Cleanup
    finalFile.close();
    tableFile.close();
    for (auto &b : barrels) b.file.close();

    cout << "\nMerge Complete!" << endl;
    cout << "Generated: " << FINAL_INDEX << " (The Data)" << endl;
    cout << "Generated: " << INDEX_TABLE << " (The Map)" << endl;

    return 0;
}