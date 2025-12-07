#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

using namespace std;

// Structure for postings
struct Posting {
    uint32_t docID;
    uint32_t freq;
};

int main() {
    const string LEXICON_FILE = "D:/Rummager/lexicon.bin";
    const string INDEX_TABLE_FILE = "D:/Rummager/index_table.bin";
    const string FINAL_INDEX_FILE = "D:/Rummager/final_index.bin";
    const string OUTPUT_FILE = "D:/Rummager/catalog.csv";

    // 1. Load lexicon
    ifstream lexFile(LEXICON_FILE, ios::binary);
    if (!lexFile) return 1;

    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));

    vector<string> lexicon(totalWords);
    for (uint32_t i = 0; i < totalWords; ++i) {
        uint32_t len;
        lexFile.read((char*)&len, sizeof(len));
        string word(len, ' ');
        lexFile.read(&word[0], len);
        lexicon[i] = word;
    }
    lexFile.close();

    // 2. Open index table
    ifstream tableFile(INDEX_TABLE_FILE, ios::binary);
    if (!tableFile) return 1;

    // 3. Open final index
    ifstream finalFile(FINAL_INDEX_FILE, ios::binary);
    if (!finalFile) return 1;

    // 4. Open output file
    ofstream outFile(OUTPUT_FILE);
    if (!outFile) return 1;
    outFile << "WordID,Word,Offset,Count\n";
    // 5. Generate catalog
    for (uint32_t i = 0; i < totalWords; ++i) {
        uint64_t offset;
        uint32_t count;
        tableFile.read((char*)&offset, sizeof(offset));
        tableFile.read((char*)&count, sizeof(count));

        // Write to catalog: WordID Word Offset Count
        outFile << i << "," << lexicon[i] << "," << offset << "," << count << "\n";
    }

    tableFile.close();
    finalFile.close();
    outFile.close();

    return 0;
}
