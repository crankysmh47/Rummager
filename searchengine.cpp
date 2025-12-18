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
#include <chrono>
#include <filesystem> // C++17
#include <queue> // NEW

using namespace std;
namespace fs = std::filesystem;

// --- CONFIGURATION ---
string BARREL_DIR = "C:\\Users\\Hank47\\Sem3\\Rummager\\barrels\\";
const string LEXICON_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\lexicon.bin";
const string LENGTHS_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_lengths.bin";
const string META_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\doc_metadata.txt";
const string PAGERANK_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\pagerank_scores.txt";
const string TRIE_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\trie.bin"; // NEW

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

// NEW: Flat Trie Node
struct FlatNode {
    char key;
    int32_t frequency; // 0 if not end of word
    int32_t childIndex; // -1 if no children
    int32_t siblingIndex; // -1 if no next sibling
    bool isEnd;
};

class BarrelSearcher {
private:
    unordered_map<string, int> lexicon;
    vector<uint32_t> docLengths;
    vector<double> pageRankScores;
    vector<DocInfo> metadata;
    vector<FlatNode> trie; // NEW
    
    double avgDL;
    uint32_t totalDocs;

public:
    BarrelSearcher() { loadMetadata(); }

    void loadMetadata() {
        cout << "--- Initializing Engine ---" << endl;
        
        lexicon.clear();
        docLengths.clear();
        pageRankScores.clear();
        metadata.clear();
        trie.clear(); // NEW

        // 1. Lexicon (Standard)
        ifstream lexFile(LEXICON_FILE, ios::binary);
        if (lexFile) {
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
        }

        // 2. Lengths (Standard)
        ifstream lenFile(LENGTHS_FILE, ios::binary);
        if (lenFile) {
            lenFile.read((char*)&totalDocs, sizeof(totalDocs));
            docLengths.resize(totalDocs);
            lenFile.read((char*)docLengths.data(), totalDocs * sizeof(uint32_t));
            lenFile.close();
        }
        
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

        // 5. Autocomplete Trie (NEW)
        ifstream tFile(TRIE_FILE, ios::binary);
        if (tFile) {
            // Get size
            tFile.seekg(0, ios::end);
            size_t size = tFile.tellg();
            tFile.seekg(0, ios::beg);
            
            size_t numNodes = size / sizeof(FlatNode);
            size_t numNodes = size / sizeof(FlatNode);
            trie.resize(numNodes);
            tFile.read((char*)trie.data(), size);
            cout << "Loaded Autocomplete Index (" << numNodes << " nodes)." << endl;
        } else {
            cout << "Warning: trie.bin not found. Autocomplete disabled." << endl;
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

    // --- OPTIMIZED QUERY FUNCTION (VECTOR INTERSECTION) ---
    vector<Result> query(string q, string categoryFilter = "", bool sortByDate = false) {
        
        // 1. Tokenize & Unique
        vector<string> rawTokens = Tokenize::tokenize(q);
        if (rawTokens.empty()) return {};

        vector<string> tokens;
        sort(rawTokens.begin(), rawTokens.end());
        unique_copy(rawTokens.begin(), rawTokens.end(), back_inserter(tokens));

        // 2. Fetch All Posting Lists & Calculate IDFs
        struct QueryTerm {
            double idf;
            vector<Posting> postings;
        };
        
        vector<QueryTerm> queryTerms;
        queryTerms.reserve(tokens.size());

        for (const string& token : tokens) {
            if (lexicon.find(token) == lexicon.end()) {
                return {}; // Short-circuit: AND logic requires all terms
            }
            
            vector<Posting> p = fetchPostings(lexicon[token]);
            if (p.empty()) return {}; // Safety check

            double n = (double)p.size();
            double idf = log((totalDocs - n + 0.5) / (n + 0.5) + 1.0);
            
            queryTerms.push_back({idf, move(p)});
        }

        // 3. Optimization: Sort by List Size (Shortest First)
        // This minimizes the initial candidate set and speeds up intersection.
        sort(queryTerms.begin(), queryTerms.end(), [](const QueryTerm& a, const QueryTerm& b) {
            return a.postings.size() < b.postings.size();
        });

        // 4. Vector Intersection (The Core Optimization)
        // Initialize candidates with the shortest list's docIDs
        vector<uint32_t> candidates;
        candidates.reserve(queryTerms[0].postings.size());
        for (const auto& p : queryTerms[0].postings) candidates.push_back(p.docID);

        // Intersect with remaining lists
        for (size_t i = 1; i < queryTerms.size(); ++i) {
            if (candidates.empty()) break; // No matches possible

            vector<uint32_t> nextCandidates;
            nextCandidates.reserve(candidates.size()); // Heuristic: can't grow

            const vector<Posting>& currentList = queryTerms[i].postings;
            
            // Two-Pointer Intersection (works because both are sorted by docID)
            size_t p1 = 0;
            size_t p2 = 0;
            
            while (p1 < candidates.size() && p2 < currentList.size()) {
                if (candidates[p1] < currentList[p2].docID) {
                    p1++;
                } else if (candidates[p1] > currentList[p2].docID) {
                    p2++;
                } else {
                    // Match found!
                    nextCandidates.push_back(candidates[p1]);
                    p1++;
                    p2++;
                }
            }
            candidates = nextCandidates;
        }

        if (candidates.empty()) return {};

        // 5. Scoring (Only for Survivors)
        vector<Result> finalRes;
        finalRes.reserve(candidates.size());

        for (uint32_t docID : candidates) {
            double docScore = 0.0;
            
            // Calculate score for each term
            for (const auto& term : queryTerms) {
                // Find freq of this term in this doc
                // Since we need random access now, we use binary search (lower_bound)
                // Note: Linear scan might be faster if k is small, but lower_bound is robust.
                
                auto it = lower_bound(term.postings.begin(), term.postings.end(), docID, 
                    [](const Posting& p, uint32_t id) { return p.docID < id; });
                
                if (it != term.postings.end() && it->docID == docID) {
                    double tf = (double)it->freq;
                    double dl = (double)docLengths[docID];
                    
                    double num = tf * (K1 + 1);
                    double den = tf + K1 * (1 - B + B * (dl / avgDL));
                    
                    docScore += term.idf * (num / den);
                }
            }

            // Category Filter
            if (!categoryFilter.empty()) {
                if (docID >= metadata.size() || metadata[docID].category.find(categoryFilter) == string::npos) {
                    continue; 
                }
            }

            // Final Ranking Score
            if (docID < pageRankScores.size()) {
                 docScore += (pageRankScores[docID] * PAGERANK_WEIGHT);
            }
            
            finalRes.push_back({docID, docScore});
        }

        // 6. Sort Results
        if (sortByDate) {
            sort(finalRes.begin(), finalRes.end(), [&](const Result& a, const Result& b) {
                string dateA = (a.docID < metadata.size()) ? metadata[a.docID].date : "0000";
                string dateB = (b.docID < metadata.size()) ? metadata[b.docID].date : "0000";
                return dateA > dateB; 
            });
        } else {
            sort(finalRes.begin(), finalRes.end(), [](const Result& a, const Result& b){
                return a.score > b.score;
            });
        }

        if (finalRes.size() > 20) finalRes.resize(20);
        return finalRes;
    }

    // --- AUTOCOMPLETE: DFS HELPER ---
    void collectSuggestions(int32_t nodeIdx, string currentWord, vector<pair<int, string>>& candidates) {
        if (nodeIdx == -1 || nodeIdx >= trie.size()) return;

        const FlatNode& node = trie[nodeIdx]; // Reference to avoid copy overhead if struct grows

        // 1. Visit Self
        if (node.frequency > 0) { // Is a valid word
            candidates.push_back({node.frequency, currentWord});
        }

        // 2. Visit Child (Deeper)
        if (node.childIndex != -1) {
             int32_t child = node.childIndex;
             while (child != -1) {
                 collectSuggestions(child, currentWord + trie[child].key, candidates);
                 child = trie[child].siblingIndex;
             }
        }
    }

    // --- AUTOCOMPLETE: MAIN FUNCTION ---
    vector<string> suggest(string prefix) {
        if (trie.empty()) return {};
        
        // Normalize prefix to lowercase
        transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

        int32_t curr = 0; 
        if (trie[0].childIndex == -1) return {};
        curr = trie[0].childIndex;

        // 1. Traverse to Prefix
        for (size_t i = 0; i < prefix.size(); ++i) {
            char c = prefix[i];
            bool found = false;
            while (curr != -1) {
                if (trie[curr].key == c) {
                    found = true;
                    // Move to child if NOT the last char of prefix
                    if (i < prefix.size() - 1) { 
                         curr = trie[curr].childIndex;
                    }
                    break;
                }
                curr = trie[curr].siblingIndex; 
            }
            if (!found) return {}; 
        }
        
        // 2. Collect ALL Candidates
        vector<pair<int, string>> candidates;
        
        // Check prefix itself
        if (trie[curr].frequency > 0) {
            candidates.push_back({trie[curr].frequency, prefix});
        }
        
        // Recurse
        int32_t child = trie[curr].childIndex;
        while (child != -1) {
            collectSuggestions(child, prefix + trie[child].key, candidates);
            child = trie[child].siblingIndex;
        }

        // 3. Sort by Frequency (Descending)
        sort(candidates.begin(), candidates.end(), [](const pair<int, string>& a, const pair<int, string>& b) {
            return a.first > b.first; 
        });

        // 4. Return Top 5
        vector<string> results;
        int count = 0;
        for (const auto& p : candidates) {
            results.push_back(p.second);
            count++;
            if (count >= 5) break;
        }
        return results;
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
    cout << "Options: /suggest <prefix>, /date, /cat:cs.AI" << endl;

    while(true) {
        // --- HOT SWAP LOGIC ---
        if (fs::exists("C:\\Users\\Hank47\\Sem3\\Rummager\\swap.signal")) {
            cout << "\n[Alignment] HOT SWAP DETECTED!" << endl;
            ifstream sig("C:\\Users\\Hank47\\Sem3\\Rummager\\swap.signal");
            string newDir;
            if (getline(sig, newDir) && !newDir.empty()) {
                BARREL_DIR = newDir;
                 if (BARREL_DIR.back() != '\\' && BARREL_DIR.back() != '/') {
                    BARREL_DIR += "\\";
                }
                cout << "Switching Barrels -> " << BARREL_DIR << endl;
                engine.loadMetadata();
            }
            sig.close();
            fs::remove("C:\\Users\\Hank47\\Sem3\\Rummager\\swap.signal");
            cout << "Hot Swap Complete." << endl;
        }

        cout << "\nQuery> ";
        if (!getline(cin, input)) break; // Stop on EOF or error
        if (input == "exit") break;

        // --- NEW: AUTOCOMPLETE COMMAND ---
        if (input.rfind("/suggest ", 0) == 0) {
            string prefix = input.substr(9);
            auto suggestions = engine.suggest(prefix);
            cout << "Suggestions: ";
            for (const auto& s : suggestions) cout << s << ", ";
            cout << endl;
            continue;
        }

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
        
        // Strip trailing space
        if (cleanQuery.back() == ' ') cleanQuery.pop_back();

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
       - We use seekg() (File Pointer) to jump directly to the data we need.
       - The "Offset Table" in the barrel allows O(1) jump.
       - This effectively treats the Hard Drive as a giant Hash Map.
*/