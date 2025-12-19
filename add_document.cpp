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
    fstream metaFile(META_FILE, ios::in | ios::out | ios::ate); // Read/Write, Start at End
    if (metaFile) {
        // Check if last char is newline
        if (metaFile.tellp() > 0) {
            metaFile.seekg(-1, ios::end);
            char c;
            metaFile.get(c);
            if (c != '\n') {
                metaFile.clear(); // Clear eof/fail bits
                metaFile.seekp(0, ios::end);
                metaFile << endl;
            }
            metaFile.seekp(0, ios::end);
        }

        // Format: ID|Title|Authors|Category|Date|OriginalID(Implicit)
        // Note: The metadata format expected by searchengine is:
        // ID|Title|Authors|Category|Date (optional)|OriginalID (optional)
        
        string title = "New Document";
        string authors = "System Updater";
        string date = "2025-01-01";
        string originalID = "new/" + to_string(newDocID);
        string category = "New";

        if (argc >= 3) title = argv[2];
        if (argc >= 4) authors = argv[3];
        if (argc >= 5) date = argv[4];
        if (argc >= 6) originalID = argv[5];
        
        // Ensure format matches searchengine's parser:
        // getline(ss, doc.originalID, '|');
        // getline(ss, doc.title, '|');
        // getline(ss, doc.authors, '|');
        // getline(ss, doc.category, '|');
        // getline(ss, doc.date, '|');
        
        // So we write: OriginalID|Title|Authors|Category|Date
        // WAIT: searchengine.cpp parser (lines 135-140) implies:
        // ID|Title|Authors|Category|Date
        
        // Let's check searchengine.cpp parser strictly:
        // getline(ss, doc.originalID, '|');
        // getline(ss, doc.title, '|');
        // getline(ss, doc.authors, '|');
        // getline(ss, doc.category, '|');
        // getline(ss, doc.date, '|');
        
        // So output must be:
        metaFile << originalID << "|" << title << "|" << authors << "|" << category << "|" << date << endl;
    }
    metaFile.close();

    cout << "Success! Document added." << endl;

    return 0;
}