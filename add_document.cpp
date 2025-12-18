#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <sstream>
#include "common.h"
#include <cstdint>

using namespace std;

// --- CONFIGURATION ---
const string LEXICON_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\lexicon.bin";
const string LENGTHS_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_lengths.bin";
const string FORWARD_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\forward_index.bin";
const string META_FILE    = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_metadata.txt";

// Helper to get file content
string readFile(const string& path) {
    ifstream t(path);
    if (!t) return "";
    stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: add_document.exe <path_to_txt_file>" << endl;
        return 1;
    }

    string inputPath = argv[1];
    string content = readFile(inputPath);
    if (content.empty()) {
        cerr << "Error: Could not read file or empty: " << inputPath << endl;
        return 1;
    }

    cout << "--- Adding Document: " << inputPath << " ---" << endl;

    // 1. LOAD LEXICON
    cout << "Loading Lexicon..." << endl;
    unordered_map<string, int> lexicon;
    fstream lexFile(LEXICON_FILE, ios::binary | ios::in | ios::out); // in/out for update
    if (!lexFile) {
        cerr << "Error: lexicon.bin not found!" << endl;
        return 1;
    }

    uint32_t totalWords;
    lexFile.read((char*)&totalWords, sizeof(totalWords));
    uint32_t originalTotalWords = totalWords;

    // Read all existing words
    for (uint32_t i = 0; i < totalWords; i++) {
        uint32_t len;
        lexFile.read((char*)&len, sizeof(len));
        string word(len, ' ');
        lexFile.read(&word[0], len);
        lexicon[word] = i; 
    }

    // 2. TOKENIZE & IDENTIFY NEW WORDS
    vector<string> tokens = Tokenize::tokenize(content);
    map<int, int> docWordFreq;
    int totalWordsInDoc = 0;
    int newWordsCount = 0;

    // We must go to the end of the lexFile to append new words
    lexFile.seekp(0, ios::end); 

    for (const string& token : tokens) {
        int id;
        if (lexicon.find(token) == lexicon.end()) {
            // NEW WORD
            id = totalWords++; 
            lexicon[token] = id;
            newWordsCount++;
            
            // Append to Lexicon File immediately
            uint32_t len = (uint32_t)token.length();
            lexFile.write((char*)&len, sizeof(len));
            lexFile.write(token.c_str(), len);
        } else {
            id = lexicon[token];
        }
        
        docWordFreq[id]++;
        totalWordsInDoc++;
    }

    // Update Lexicon Header if changed
    if (newWordsCount > 0) {
        lexFile.seekp(0, ios::beg);
        lexFile.write((char*)&totalWords, sizeof(totalWords)); // New Count
        cout << "Added " << newWordsCount << " new words to Lexicon." << endl;
    }
    lexFile.close();

    // 3. DETERMINE NEW DOC ID & UPDATE LENGTHS
    fstream lenFile(LENGTHS_FILE, ios::binary | ios::in | ios::out);
    if (!lenFile) {
        cerr << "Error: doc_lengths.bin not found!" << endl;
        return 1;
    }
    uint32_t totalDocs;
    lenFile.read((char*)&totalDocs, sizeof(totalDocs));
    
    uint32_t newDocID = totalDocs;
    totalDocs++;
    
    // Update header
    lenFile.seekp(0, ios::beg);
    lenFile.write((char*)&totalDocs, sizeof(totalDocs));
    
    // Append new length to end
    lenFile.seekp(0, ios::end);
    uint32_t docLen = (uint32_t)totalWordsInDoc;
    lenFile.write((char*)&docLen, sizeof(docLen));
    lenFile.close();
    
    cout << "Assigned DocID: " << newDocID << endl;

    // 4. APPEND TO FORWARD INDEX
    ofstream fwdFile(FORWARD_FILE, ios::binary | ios::app); // Append mode
    if (!fwdFile) {
        cerr << "Error: forward_index.bin not found!" << endl;
        return 1;
    }

    uint32_t uDocID = newDocID;
    uint32_t uTotal = (uint32_t)totalWordsInDoc;
    uint32_t uUnique = (uint32_t)docWordFreq.size();

    fwdFile.write((char*)&uDocID, sizeof(uDocID));
    fwdFile.write((char*)&uTotal, sizeof(uTotal));
    fwdFile.write((char*)&uUnique, sizeof(uUnique));

    for (const auto& pair : docWordFreq) {
        uint32_t uWordID = (uint32_t)pair.first;
        uint32_t uFreq = (uint32_t)pair.second;
        fwdFile.write((char*)&uWordID, sizeof(uWordID));
        fwdFile.write((char*)&uFreq, sizeof(uFreq));
    }
    fwdFile.close();

    // 5. UPDATE METADATA
    ofstream metaFile(META_FILE, ios::app);
    if (metaFile) {
        // Format: ID|Title|Authors|Category|Date
        // We'll use the filename as title/ID reference
        string filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);
        metaFile << newDocID << "|New Doc: " << filename << "|System Updater|New|2025-01-01" << endl;
    }
    metaFile.close();

    cout << "Success! Document added." << endl;

    // --- PHASE 3: HOT SWAPPING TRIGGER ---
    cout << "--- Starting Hot-Swap Pipeline ---" << endl;
    
    // 1. Invert (Full Forward Index -> Inverted Index)
    cout << "Running Invert..." << endl;
    system(".\\INVERT.exe"); 
    
    // Note: The user environment has INVERT.exe (uppercase) in the file list.
    // I should check exact name. list_dir showed INVERT.exe.
    // Also "invert.cpp". I should maybe compile invert.cpp to invert.exe to be sure?
    // list_dir showed "INVERT.exe" (194KB) and "invert.cpp".
    // I previously compiled create_barrels which is create_barrels.exe.
    // I will try to run INVERT.exe.
    
    // 2. Create Barrels (Staging)
    string stagingDir = "C:\\Users\\Hank47\\Sem3\\Rummager\\barrels_staging";
    string cmd = ".\\create_barrels.exe \"" + stagingDir + "\"";
    cout << "Running Create Barrels (Staging)..." << endl;
    system(cmd.c_str());

    // 3. Create Signal File
    ofstream signalFile("C:\\Users\\Hank47\\Sem3\\Rummager\\swap.signal");
    signalFile << stagingDir; // Write staging path to signal
    signalFile.close();
    
    cout << "Signal 'swap.signal' created. Search Engine should pick it up." << endl;

    return 0;
}