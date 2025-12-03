#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

using namespace std;


unordered_map<string, int> lexicon;
int nextWordID = 1;


int getWordID(const string& word) {
    if (lexicon.find(word) == lexicon.end()) {
        lexicon[word] = nextWordID++;
    }
    return lexicon[word];
}


void saveLexicon() {
    ofstream outFile("lexicon.txt");
    for (const auto& pair : lexicon) {
        outFile << pair.first << " " << pair.second << "\n";
    }
    outFile.close();
    cout << "Lexicon saved with " << lexicon.size() << " unique words." << endl;
}

int main() {

    ifstream infile("corpus.txt"); 
    if (!infile.is_open()) {
        cerr << "Error: Could not open corpus.txt" << endl;
        return 1;
    }


    ofstream outfile("forward_index.bin", ios::binary);

    string line;
    int docsProcessed = 0;

    cout << "Starting Forward Indexing..." << endl;


    while (getline(infile, line)) {
        stringstream ss(line);
        
        int docID;
        ss >> docID; // Assume first token is DocID

        string word;
        // Map to store frequency for THIS document only
        // map<WordID, Frequency>
        map<int, int> docWordFreq; 
        int totalWordsInDoc = 0;

        // 4. Tokenize and Count Frequencies
        while (ss >> word) {
            int id = getWordID(word);
            docWordFreq[id]++;
            totalWordsInDoc++;
        }

        // 5. Write to Binary File
        // Structure: [DocID] [TotalWords] [UniqueCount] [WordID, Freq]...
        
        int uniqueCount = docWordFreq.size();

        outfile.write(reinterpret_cast<char*>(&docID), sizeof(int));
        outfile.write(reinterpret_cast<char*>(&totalWordsInDoc), sizeof(int)); // For TF-IDF later
        outfile.write(reinterpret_cast<char*>(&uniqueCount), sizeof(int));     // For Reading loop

        for (auto const& [id, freq] : docWordFreq) {
            outfile.write(reinterpret_cast<const char*>(&id), sizeof(int));
            outfile.write(reinterpret_cast<const char*>(&freq), sizeof(int));
        }

        docsProcessed++;
        if (docsProcessed % 1000 == 0) {
            cout << "Processed " << docsProcessed << " documents..." << endl;
        }
    }

    infile.close();
    outfile.close();

   
    saveLexicon();

    cout << "Done! Forward Index generated." << endl;
    return 0;
}