#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <sys/stat.h> // For mkdir

using namespace std;

// --- CONFIGURATION ---
const string INPUT_FILE = "inverted_index.bin"; // Your big file
const string OUTPUT_DIR = "barrels/";           // Output folder
const uint32_t WORDS_PER_BARREL = 10000;        // Range size (0-9999, etc.)

// Define your Posting structure size here so the copier knows how many bytes to read
// Example: If you have DocID (4 bytes) + Freq (4 bytes), size is 8.
struct Posting {
    uint32_t docID;
    uint32_t frequency; // Uncomment if you have this
}; 

int main() {
    // 1. Prepare
    ifstream inFile(INPUT_FILE, ios::binary);
    if (!inFile) { cerr << "Error: Cannot open " << INPUT_FILE << endl; return 1; }
    
    #ifdef _WIN32
        system(("mkdir " + OUTPUT_DIR).c_str());
    #else
        system(("mkdir -p " + OUTPUT_DIR).c_str());
    #endif

    cout << "Partitioning " << INPUT_FILE << " into barrels of size " << WORDS_PER_BARREL << "..." << endl;

    ofstream currentBarrel;
    int32_t currentBarrelIdx = -1;

    uint32_t wordID;
    uint32_t count; // Number of postings for this word

    // 2. Loop through the Monolith
    while (inFile.read((char*)&wordID, sizeof(wordID))) {
        
        // Read the count (Document Frequency)
        inFile.read((char*)&count, sizeof(count));

        // Calculate needed barrel
        int32_t targetBarrelIdx = wordID / WORDS_PER_BARREL;

        // Switch files if we moved to a new range
        if (targetBarrelIdx != currentBarrelIdx) {
            if (currentBarrel.is_open()) currentBarrel.close();
            
            string bName = OUTPUT_DIR + "barrel_" + to_string(targetBarrelIdx) + ".bin";
            currentBarrel.open(bName, ios::binary | ios::app); // Append mode in case of unsorted input (rare)
            
            if (!currentBarrel) { cerr << "Failed to create " << bName << endl; return 1; }
            
            cout << "Writing to Barrel " << targetBarrelIdx << " (Starts at WordID " << wordID << ")" << endl;
            currentBarrelIdx = targetBarrelIdx;
        }

        // 3. Write WordID and Count to the barrel
        currentBarrel.write((char*)&wordID, sizeof(wordID));
        currentBarrel.write((char*)&count, sizeof(count));

        // 4. Efficiently Copy Postings (Bulk Copy)
        if (count > 0) {
            size_t bytesToCopy = count * sizeof(Posting);
            vector<char> buffer(bytesToCopy); // Temp buffer
            
            // Read from Source
            inFile.read(buffer.data(), bytesToCopy);
            
            // Write to Barrel
            currentBarrel.write(buffer.data(), bytesToCopy);
        }
    }

    if (currentBarrel.is_open()) currentBarrel.close();
    inFile.close();

    cout << "Partitioning Complete." << endl;
    return 0;
}