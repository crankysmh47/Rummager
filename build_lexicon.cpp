#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <algorithm>
#include <cstdint> // Required for fixed-width integers (uint32_t)

using namespace std;

const unordered_set<string> STOPWORDS = {
    "a", "about", "above", "after", "again", "against", "all", "am", "an", "and",
    "any", "are", "aren't", "as", "at", "be", "because", "been", "before", "being",
    "below", "between", "both", "but", "by", "can't", "cannot", "could", "couldn't",
    "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during",
    "each", "few", "for", "from", "further", "had", "hadn't", "has", "hasn't",
    "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here",
    "here's", "hers", "herself", "him", "himself", "his", "how", "how's", "i",
    "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's",
    "its", "itself", "let's", "me", "more", "most", "mustn't", "my", "myself",
    "no", "nor", "not", "of", "off", "on", "once", "only", "or", "other", "ought",
    "our", "ours", "ourselves", "out", "over", "own", "same", "shan't", "she",
    "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such",
    "than", "that", "that's", "the", "their", "theirs", "them", "themselves",
    "then", "there", "there's", "these", "they", "they'd", "they'll", "they're",
    "they've", "this", "those", "through", "to", "too", "under", "until", "up",
    "very", "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were",
    "weren't", "what", "what's", "when", "when's", "where", "where's", "which",
    "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would",
    "wouldn't", "you", "you'd", "you'll", "you're", "you've", "your", "yours",
    "yourself", "yourselves"
};


class Tokenize {
public:
    static vector<string> tokenize(const string& text) {
        vector<string> tokens;
        string currentToken;
        // Optimization: Reserve estimated memory to reduce reallocations
        tokens.reserve(text.length() / 6); 

        for (char c : text) {
            if (isalnum(c)) {
                currentToken += tolower(c);
            } else {
                if (!currentToken.empty()) {
                    // Check Stopwords
                    if (STOPWORDS.find(currentToken) == STOPWORDS.end()) {
                        tokens.push_back(currentToken);
                    }
                    currentToken = "";
                }
            }
        }

        // Catch the last token if the string doesn't end with punctuation
        if (!currentToken.empty()) {
            if (STOPWORDS.find(currentToken) == STOPWORDS.end()) {
                tokens.push_back(currentToken);
            }
        }
        
        return tokens;
    }
};

class Lexicon {
private:
    unordered_map<string, int> wordToID;
    vector<string> idToWord;

public:
    int getID(const string& word) {
        if (wordToID.find(word) != wordToID.end()) {
            return wordToID[word];
        }
        int newID = idToWord.size();
        wordToID[word] = newID;
        idToWord.push_back(word);
        return newID;
    }

    // Save using fixed-width integers (uint32_t) for binary safety
    void saveBinary(const string& filename) {
        ofstream outFile(filename, ios::binary);
        if (!outFile) {
            cerr << "Error: Could not write to " << filename << endl;
            return;
        }

        uint32_t size = (uint32_t)idToWord.size();
        outFile.write((char*)&size, sizeof(size));

        for (const string& word : idToWord) {
            uint32_t len = (uint32_t)word.size();
            outFile.write((char*)&len, sizeof(len));
            outFile.write(word.c_str(), len);
        }
        outFile.close();
    }
};


int main() {
    const string CLEAN_DATA_FILE = "clean_dataset.txt";
    const string LEXICON_FILE = "lexicon.bin";
    

    const int MAX_DOCS = -1; 

    Lexicon lexicon;
    ifstream file(CLEAN_DATA_FILE);

    if (!file.is_open()) {
        cerr << "Error: Could not open " << CLEAN_DATA_FILE << endl;
        cerr << "Make sure to run the python preprocess script first!" << endl;
        return 1;
    }

    string line;
    int count = 0;
    cout << "Reading clean dataset and building Lexicon..." << endl;

    while (getline(file, line)) {
        if (MAX_DOCS != -1 && count >= MAX_DOCS) break;

        // The format is: DOC_ID [TAB] CONTENT
        // We find the first TAB to separate ID from content
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) continue; // Skip malformed lines

        // We don't need docID for the lexicon, but we extract content
        // string docID = line.substr(0, tabPos); 
        string content = line.substr(tabPos + 1);

        vector<string> tokens = Tokenize::tokenize(content);

        for (const string& token : tokens) {
            lexicon.getID(token);
        }

        count++;
        if (count % 10000 == 0) cout << "Processed " << count << " lines...\r" << flush;
    }

    cout << "\nDataset pass complete. Saving Lexicon..." << endl;
    lexicon.saveBinary(LEXICON_FILE);
    
    cout << "Done! Lexicon saved to " << LEXICON_FILE << endl;
    cout << "Total unique words indexed: " << lexicon.getID("DUMMY_CHECK") - 1 << endl;

    return 0;
}







/*main.cpp (The Builder)Exact Function:This program reads the clean_dataset.txt,
 splits the text into words (tokenization), filters out useless words (stopwords),
  assigns a unique Integer ID to every unique word (Lexicon building),
   and saves the map to a binary file.Key Optimizations:uint32_t (Fixed-Width Integers)
   :Why: Using int can vary between systems (sometimes 16-bit, usually 32-bit). uint32_t guarantees 4 bytes. 
   This makes the .bin file portable and prevents corruption if you move the index between different operating 
   
   systems.tokens.reserve():Why: Vectors resize dynamically.
   If a vector grows beyond its capacity, it has to find new memory, copy everything over, and delete the old memory.
   By guessing the size (text.length() / 6), we reserve memory upfront, preventing costly reallocations.unordered_map 
   
   (Hash Map):Why: This provides $O(1)$ average time complexity for lookups. 
   Checking if a word exists or getting its ID is instant, regardless of whether the lexicon has 10 words or 1,000,000
   
   words.unordered_set for Stopwords:Why: Searching a vector for stopwords is $O(N)$. 
   Searching a set is $O(1)$. This drastically speeds up tokenization.
   
   
   Binary Saving (write((char*)&size...):Why: Writing the number 123456 as text takes 6 bytes ("1", "2", "3"...).
   Writing it as binary takes 4 bytes. Binary writing is significantly faster (no string conversion) and results in a smaller file size.*/