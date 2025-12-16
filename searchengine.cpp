#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <chrono>
#include "common.h" // Your Tokenizer

using namespace std;

// --- CONFIGURATION ---
const string LEXICON_FILE = "lexicon.bin";
const string INV_IDX_FILE = "inverted_index.bin";
const string LENGTHS_FILE = "doc_lengths.bin";
const string DOC_MAP_FILE = "docid_map.txt";
// You need to export PageRank as "IntID Score" text file for easy C++ loading, 
// OR simpler: just load the "pagerank_scores.txt" (IntID -> Score) directly!
const string PAGERANK_FILE = "pagerank_scores.txt"; 

const double K1 = 1.5;
const double B = 0.75;
const double PAGERANK_WEIGHT = 10000.0;

struct Posting {
    uint32_t docID;
    uint32_t freq;
};

struct Result {
    uint32_t docID;
    double score;
};

class SearchServer {
private:
    // Memory Structures
    unordered_map<string, int> lexicon;
    vector<uint32_t> docLengths;
    vector<double> pageRankScores;
    vector<string> docIDMap; // IntID -> StringID
    
    // Index Offsets (Where does word ID X start in the file?)
    vector<long long> indexOffsets;
    
    double avgDL;
    uint32_t totalDocs;
    ifstream invFile;

public:
    SearchServer() {
        loadData();
    }

    void loadData() {
        cout << "--- Loading High-Performance Engine ---" << endl;

        // 1. Load Lexicon
        cout << "Loading Lexicon...";
        ifstream lexFile(LEXICON_FILE, ios::binary);
        uint32_t totalWords;
        lexFile.read((char*)&totalWords, sizeof(totalWords));
        indexOffsets.resize(totalWords, -1); // Initialize offsets

        for (uint32_t i = 0; i < totalWords; i++) {
            uint32_t len;
            lexFile.read((char*)&len, sizeof(len));
            string word(len, ' ');
            lexFile.read(&word[0], len);
            lexicon[word] = i;
        }
        cout << " OK (" << totalWords << " words)" << endl;

        // 2. Load Doc Lengths
        cout << "Loading Lengths...";
        ifstream lenFile(LENGTHS_FILE, ios::binary);
        lenFile.read((char*)&totalDocs, sizeof(totalDocs));
        docLengths.resize(totalDocs);
        
        long long sumLen = 0;
        for (uint32_t i = 0; i < totalDocs; i++) {
            lenFile.read((char*)&docLengths[i], sizeof(docLengths[i]));
            sumLen += docLengths[i];
        }
        avgDL = (double)sumLen / totalDocs;
        cout << " OK (AvgDL: " << avgDL << ")" << endl;

        // 3. Load PageRank (IntID -> Score)
        // We assume pagerank_scores.txt is formatted: "IntID Score"
        cout << "Loading PageRank...";
        pageRankScores.resize(totalDocs, 0.0); // Default 0
        ifstream prFile(PAGERANK_FILE);
        int prID;
        double prScore;
        while(prFile >> prID >> prScore) {
            if(prID < totalDocs) pageRankScores[prID] = prScore;
        }
        cout << " OK." << endl;

        // 4. Load Doc Map (IntID -> StringID)
        cout << "Loading ID Map...";
        docIDMap.resize(totalDocs);
        ifstream mapFile(DOC_MAP_FILE);
        int intID;
        string strID;
        while(mapFile >> intID >> strID) {
            if(intID < totalDocs) docIDMap[intID] = strID;
        }
        cout << " OK." << endl;

        // 5. Index the Inverted File (Store Offsets)
        cout << "Indexing Offsets...";
        invFile.open(INV_IDX_FILE, ios::binary);
        
        // Skip header (TotalWords)
        uint32_t checkWords;
        invFile.read((char*)&checkWords, sizeof(checkWords));
        
        // We must scan the file to find where each list starts
        // But since we wrote it in Order (WordID 0, 1, 2...), we can just read sequentially!
        // Wait... In inverted_index.cpp, we looped 0..TotalWords. 
        // So the file IS sequential. We just need to skip data blocks.
        
        for(uint32_t i=0; i < totalWords; i++) {
            indexOffsets[i] = invFile.tellg(); // Save position
            
            uint32_t listSize;
            invFile.read((char*)&listSize, sizeof(listSize));
            
            // Skip the postings (8 bytes per posting)
            invFile.seekg(listSize * sizeof(Posting), ios::cur);
        }
        cout << " OK." << endl;
    }

    vector<Result> query(string userQuery) {
        vector<string> tokens = Tokenize::tokenize(userQuery);
        
        // Accumulator for scores (Map is slow, but usually sparse. 
        // For max speed with dense IDs, use a vector<double> reset per query, 
        // but that uses RAM = 8 bytes * 2Million = 16MB per query. Efficient enough!)
        
        // Let's use an unordered_map for sparsity, or a vector if RAM allows.
        // For 2M docs, a vector is faster than a map if we hit many docs.
        // Let's stick to map for simplicity, or vector for raw speed.
        // Let's use Map for now to be safe on RAM.
        unordered_map<uint32_t, double> scores; 

        for(const string& token : tokens) {
            if(lexicon.find(token) == lexicon.end()) continue;
            
            int wordID = lexicon[token];
            long long offset = indexOffsets[wordID];
            
            // Jump to data
            invFile.clear(); // Clear EOF flags if any
            invFile.seekg(offset);
            
            uint32_t listSize;
            invFile.read((char*)&listSize, sizeof(listSize));
            
            // Load Postings
            vector<Posting> postings(listSize);
            invFile.read((char*)postings.data(), listSize * sizeof(Posting));
            
            // Calculate IDF
            // idf = log( (N - n + 0.5) / (n + 0.5) + 1 )
            double n = (double)listSize;
            double idf = log( (totalDocs - n + 0.5) / (n + 0.5) + 1.0 );
            
            // BM25 Loop
            for(const auto& p : postings) {
                double tf = (double)p.freq;
                double docLen = (double)docLengths[p.docID];
                
                double numerator = tf * (K1 + 1);
                double denominator = tf + K1 * (1 - B + B * (docLen / avgDL));
                
                double termScore = idf * (numerator / denominator);
                scores[p.docID] += termScore;
            }
        }

        // Convert to Result Vector & Add PageRank
        vector<Result> finalResults;
        finalResults.reserve(scores.size());
        
        for(auto const& [docID, bm25] : scores) {
            double finalScore = bm25 + (pageRankScores[docID] * PAGERANK_WEIGHT);
            finalResults.push_back({docID, finalScore});
        }

        // Sort (Descending)
        sort(finalResults.begin(), finalResults.end(), [](const Result& a, const Result& b) {
            return a.score > b.score;
        });

        if(finalResults.size() > 20) finalResults.resize(20);
        return finalResults;
    }

    string getDocName(uint32_t docID) {
        if(docID < docIDMap.size()) return docIDMap[docID];
        return "Unknown";
    }
};

int main() {
    SearchServer engine;
    
    string input;
    cout << "\nReady for queries (type 'exit' to quit):" << endl;
    
    while(true) {
        cout << "> ";
        getline(cin, input);
        if(input == "exit") break;
        
        auto start = chrono::high_resolution_clock::now();
        vector<Result> results = engine.query(input);
        auto end = chrono::high_resolution_clock::now();
        
        chrono::duration<double> diff = end - start;
        
        cout << "Found " << results.size() << " results in " << diff.count() << " s." << endl;
        for(const auto& res : results) {
            cout << "ID: " << engine.getDocName(res.docID) 
                 << " \t Score: " << res.score << endl;
        }
    }
    return 0;
}