
#include<iostream>
#include<string>
#include <algorithm>
#include<fstream>
#include<vector>
#include<unordered_map>
#include <unordered_set>

using namespace std;





struct Arxiver{
    string id, authors, title, categories, abstract, update_date ;
};

class Parser{
    public:
    Arxiver parse(const string& jsonLine) {
        Arxiver article;

        article.id = extractField(jsonLine, "id");
        article.title = extractField(jsonLine, "title");
        article.abstract = extractField(jsonLine, "abstract");
        article.authors = extractField(jsonLine, "authors");
        article.categories = extractField(jsonLine, "categories");
        article.update_date = extractField(jsonLine, "update_date");

        return article;
    }
    private:
        string extractField(const string& json, const string& key) {
            string keyPattern = "\"" + key + "\"";
            
            size_t keyPos = json.find(keyPattern);
            if (keyPos == string::npos) return "";

            size_t colonPos = json.find(':', keyPos);
            if (colonPos == string::npos) return "";

            size_t startQuote = json.find('"', colonPos);
            if (startQuote == string::npos) return "";

            string value = "";
            bool isEscaped = false;
            
            for (size_t i = startQuote + 1; i < json.length(); i++) {
                char c = json[i];
                
                if (isEscaped) {
                    value += c;
                    isEscaped = false;
                } else if (c == '\\') {
                    isEscaped = true;
                } else if (c == '"') {
                    break; // End of value
                } else {
                    value += c;
                }
            }
            
            //Remove newline characters and spaces
            replace(value.begin(), value.end(), '\n', ' ');
            return value;
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
    
        string getWord(int id) {
            if (id >= 0 && (size_t)id < idToWord.size()) return idToWord[id];
            return "";
        }
    
        void saveBinary(const string& filename) {
            ofstream outFile(filename, ios::binary);
            size_t size = idToWord.size();
            outFile.write((char*)&size, sizeof(size));
            for (const string& word : idToWord) {
                size_t len = word.size();
                outFile.write((char*)&len, sizeof(len));
                outFile.write(word.c_str(), len);
            }
            outFile.close();
        }
    
        void loadBinary(const string& filename) {
            ifstream inFile(filename, ios::binary);
            if (!inFile) return;
            size_t size;
            inFile.read((char*)&size, sizeof(size));
            idToWord.clear();
            wordToID.clear();
            for (size_t i = 0; i < size; i++) {
                size_t len;
                inFile.read((char*)&len, sizeof(len));
                string word(len, '\0');
                inFile.read(&word[0], len);
                idToWord.push_back(word);
                wordToID[word] = i;
            }
            inFile.close();
        }
};


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
        
        // Reserve memory (Optimization)
        tokens.reserve(text.length() / 5);

        for (char c : text) {
            if (isalnum(c)) {
                currentToken += tolower(c);
            } else {
                if (!currentToken.empty()) {
                    // Use the global STOPWORDS set
                    if (STOPWORDS.find(currentToken) == STOPWORDS.end()) {
                        tokens.push_back(currentToken);
                    }
                    currentToken = "";
                }
            }
        }

        if (!currentToken.empty()) {
            if (STOPWORDS.find(currentToken) == STOPWORDS.end()) {
                tokens.push_back(currentToken);
            }
        }
        
        return tokens;
    }
};



int main() {
    const string DATASET_FILE = "D:\\Web Downloads\\arxiv-metadata-oai-snapshot.json";
    const string LEXICON_FILE = "lexicon.bin";
    const int MAX_DOCS = 10; 

    Parser parser;
    Lexicon lexicon;

    ifstream file(DATASET_FILE);
    if (!file.is_open()) {
        cerr << "Error opening file." << endl;
        return 1;
    }

    string line;
    int count = 0;

    cout << "Building Lexicon..." << endl;

    while (getline(file, line)) {
        if (MAX_DOCS != -1 && count >= MAX_DOCS) break;

        Arxiver doc = parser.parse(line);
        
        string fullText = doc.title + " " + doc.authors + " " + doc.abstract + " " + doc.categories;

        vector<string> tokens = Tokenize::tokenize(fullText);

        for (const string& token : tokens) {
            lexicon.getID(token);
        }

        count++;
        if (count % 10000 == 0) cout << "Processed " << count << "\r" << flush;
    }
    
    lexicon.saveBinary(LEXICON_FILE);
    
    cout << "\nDone. Lexicon saved to " << LEXICON_FILE << endl;
    cout << "Total unique words: " << lexicon.getID("DUMMY_WORD_CHECK") - 1 << endl;

    return 0;
}