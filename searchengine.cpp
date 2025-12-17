#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "common.h"
#include <cstdint>
#include<chrono>

using namespace std;

// --- CONFIGURATION ---
const string BARREL_DIR = "C:\\Users\\Hank47\\Sem3\\Rummager\\barrels\\";
const string LEXICON_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\lexicon.bin";
const string LENGTHS_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_lengths.bin";
const string META_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_metadata.txt"; // The new file
const string PAGERANK_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\pagerank_scores.txt";

const uint32_t WORDS_PER_BARREL = 50000;
const double K1 = 1.5;
const double B = 0.75;
const double PAGERANK_WEIGHT = 50.0;

struct Posting { uint32_t docID; uint32_t freq; };
struct Result { uint32_t docID; double score; };

// NEW: Struct to hold full paper details
struct DocInfo {
    string originalID;
    string title;
    string authors;
    string category;
    string date;
};

class BarrelSearcher {
private:
    unordered_map<string, int> lexicon;
    vector<uint32_t> docLengths;
    vector<double> pageRankScores;
    
    // NEW: Vector of Structs instead of just strings
    vector<DocInfo> metadata;
    
    double avgDL;
    uint32_t totalDocs;

public:
    BarrelSearcher() { loadMetadata(); }

    void loadMetadata() {
        cout << "--- Initializing Engine ---" << endl;
        
        // 1. Lexicon (Standard)
        ifstream lexFile(LEXICON_FILE, ios::binary);
        uint32_t totalWords;
        lexFile.read((char*)&totalWords, sizeof(totalWords));
        for (uint32_t i = 0; i < totalWords; i++) {
            uint32_t len;
            lexFile.read((char*)&len, sizeof(len));
            string word(len, ' ');
            lexFile.read(&word[0], len);
            lexicon[word] = i;
        }
        lexFile.close();

        // 2. Lengths (Standard)
        ifstream lenFile(LENGTHS_FILE, ios::binary);
        lenFile.read((char*)&totalDocs, sizeof(totalDocs));
        docLengths.resize(totalDocs);
        lenFile.read((char*)docLengths.data(), totalDocs * sizeof(uint32_t));
        lenFile.close();
        
        long long sum = 0;
        for (uint32_t l : docLengths) sum += l;
        avgDL = (totalDocs > 0) ? (double)sum / totalDocs : 0;

        // 3. Metadata (UPDATED)
        cout << "Loading Metadata...";
        ifstream mFile(META_FILE);
        if (mFile) {
            string line;
            while (getline(mFile, line)) {
                DocInfo doc;
                stringstream ss(line);
                string segment;
                
                // Parse "ID|Title|Authors|Category|Date"
                getline(ss, doc.originalID, '|');
                getline(ss, doc.title, '|');
                getline(ss, doc.authors, '|');
                getline(ss, doc.category, '|');
                getline(ss, doc.date, '|');
                
                metadata.push_back(doc);
            }
            cout << " Loaded " << metadata.size() << " docs." << endl;
        }

        // 4. PageRank (Standard)
        pageRankScores.resize(totalDocs, 0.0);
        ifstream prFile(PAGERANK_FILE);
        if (prFile) {
            int id; double score;
            while(prFile >> id >> score) {
                if(id < (int)totalDocs) pageRankScores[id] = score;
            }
        }
    }

    // --- BARREL FETCH (Standard) ---
    vector<Posting> fetchPostings(int globalWordID) {
        int barrelID = globalWordID / WORDS_PER_BARREL;
        int localID = globalWordID % WORDS_PER_BARREL;
        string fname = BARREL_DIR + "barrel_" + to_string(barrelID) + ".bin";
        ifstream file(fname, ios::binary);
        if (!file) return {}; 

        long long offsetPos = localID * sizeof(long long);
        file.seekg(offsetPos);
        long long dataOffset;
        file.read((char*)&dataOffset, sizeof(dataOffset));
        if (dataOffset == 0) return {}; 

        file.seekg(dataOffset);
        uint32_t listSize;
        file.read((char*)&listSize, sizeof(listSize));
        vector<Posting> results(listSize);
        file.read((char*)results.data(), listSize * sizeof(Posting));
        return results;
    }

    // --- QUERY (Standard) ---
    // --- UPDATED QUERY FUNCTION ---
    // Now accepts optional filter and sort flags
    vector<Result> query(string q, string categoryFilter = "", bool sortByDate = false) {
        
        vector<string> tokens = Tokenize::tokenize(q);
        unordered_map<uint32_t, double> scores;

        // 1. Standard BM25 Retrieval
        for (const string& token : tokens) {
            if (lexicon.find(token) == lexicon.end()) continue;
            int id = lexicon[token];
            
            vector<Posting> postings = fetchPostings(id);
            if (postings.empty()) continue;
            
            double n = (double)postings.size();
            double idf = log((totalDocs - n + 0.5) / (n + 0.5) + 1.0);

            for (const auto& p : postings) {
                // --- FILTERING STEP ---
                // If the user asked for a category (e.g., "cs.LG"), check metadata NOW.
                // This saves us from calculating math for irrelevant papers.
                if (!categoryFilter.empty()) {
                    // Check if our metadata exists for this ID
                    if (p.docID < metadata.size()) {
                        // "cs.LG" is often inside "cs.LG cs.AI stat.ML"
                        // We check if the substring exists.
                        if (metadata[p.docID].category.find(categoryFilter) == string::npos) {
                            continue; // Skip this doc!
                        }
                    }
                }

                double tf = (double)p.freq;
                double dl = (double)docLengths[p.docID];
                double num = tf * (K1 + 1);
                double den = tf + K1 * (1 - B + B * (dl / avgDL));
                
                scores[p.docID] += idf * (num / den);
            }
        }

        vector<Result> finalRes;
        for (auto& pair : scores) {
            double finalScore = pair.second + (pageRankScores[pair.first] * PAGERANK_WEIGHT);
            finalRes.push_back({pair.first, finalScore});
        }
        
        // --- SORTING STEP ---
        if (sortByDate) {
            // Sort by Date (Descending: Newest first)
            sort(finalRes.begin(), finalRes.end(), [&](const Result& a, const Result& b) {
                // We access the metadata vector to compare date strings
                // "2023-10-27" > "2020-01-01" works perfectly with string comparison
                string dateA = (a.docID < metadata.size()) ? metadata[a.docID].date : "0000";
                string dateB = (b.docID < metadata.size()) ? metadata[b.docID].date : "0000";
                return dateA > dateB; 
            });
        } else {
            // Default: Sort by Relevance Score
            sort(finalRes.begin(), finalRes.end(), [](const Result& a, const Result& b){
                return a.score > b.score;
            });
        }

        if (finalRes.size() > 20) finalRes.resize(20);
        return finalRes;
    }

    void printDoc(uint32_t id, double score) {
        if (id >= metadata.size()) return;
        const DocInfo& doc = metadata[id];
        
        cout << "------------------------------------------------" << endl;
        cout << " [" << score << "] " << doc.title << endl;
        cout << "       Authors: " << doc.authors.substr(0, 80) << (doc.authors.size()>80?"...":"") << endl;
        cout << "       Category: " << doc.category << " | Date: " << doc.date << endl;
        
        // --- LINK GENERATION ---
        cout << "       Link: https://arxiv.org/abs/" << doc.originalID << endl;
    }
};

int main() {
    BarrelSearcher engine;
    string input;
    
    cout << "\n=== arXiv Search Engine ===" << endl;
    cout << "Options: /date (sort by date), /cat:cs.AI (filter by category)" << endl;

    while(true) {
        cout << "\nQuery> ";
        if (!getline(cin, input)) break; // Stop on EOF or error
        if (input == "exit") break;

        bool sortDate = false;
        string catFilter = "";
        string cleanQuery = "";

        // Simple Command Parsing
        // Example Input: "machine learning /date /cat:cs.LG"
        stringstream ss(input);
        string word;
        while(ss >> word) {
            if (word == "/date") {
                sortDate = true;
            } else if (word.rfind("/cat:", 0) == 0) { // Starts with /cat:
                catFilter = word.substr(5); // Extract "cs.LG"
            } else {
                cleanQuery += word + " ";
            }
        }

        if (cleanQuery.empty()) continue;

        cout << "Searching for: '" << cleanQuery << "'";
        if (!catFilter.empty()) cout << " [Filter: " << catFilter << "]";
        if (sortDate) cout << " [Sorted by Date]";
        cout << "..." << endl;

        auto start = chrono::high_resolution_clock::now();
        // CALL THE UPDATED FUNCTION
        auto results = engine.query(cleanQuery, catFilter, sortDate);
        auto end = chrono::high_resolution_clock::now();
        
        cout << "Found " << results.size() << " results in " 
             << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms." << endl;
        
        for (const auto& r : results) {
            engine.printDoc(r.docID, r.score);
        }
    }
    return 0;
}

/*
    ========================================================================================
    EDUCATIONAL SUMMARY: SEARCH ENGINE ARCHITECTURE
    ========================================================================================
    
    1. THE RETRIEVAL PROCESS (BM25)
       - We iterate through query terms.
       - For each term, we fetch its "Posting List" (Docs that contain it).
       - We calculate a score for each Doc-Term pair using BM25:
         - TF (Term Freq): How often word appears in doc (Diminishing returns).
         - IDF (Inv Doc Freq): How rare is the word (Rare = High Value).
         - DL (Doc Length): Penalize very long documents (Normalization).
    
    2. THE RANKING (COMBINATION)
       - Final Score = BM25_Score + (PageRank * Weight).
       - BM25 measures "Relevance" (Content).
       - PageRank measures "Authority" (Graph Structure).
    
    3. EFFICIENCY (SEEKING)
       - We do NOT load the entire index into RAM.
       - We use `seekg()` (File Pointer) to jump directly to the data we need.
       - The "Offset Table" in the barrel allows O(1) jump.
       - This effectively treats the Hard Drive as a giant Hash Map.
*/