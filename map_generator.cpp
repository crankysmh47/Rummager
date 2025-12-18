#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// Configuration
const string DATASET_FILE = "C:\\Users\\Hank47\\Sem3\\Rummager\\clean_dataset.txt";
const string OUTPUT_FILE  = "C:\\Users\\Hank47\\Sem3\\Rummager\\id_map.txt";

int main() {
    ifstream infile(DATASET_FILE);
    if (!infile) {
        cerr << "Error: Could not open " << DATASET_FILE << endl;
        return 1;
    }

    vector<string> allIDs;
    string line;
    long long lineCount = 0;

    cout << "Reading dataset to extract IDs..." << endl;
    while (getline(infile, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos != string::npos) {
            string id = line.substr(0, tabPos);
            allIDs.push_back(id);
        }
        lineCount++;
        if (lineCount % 100000 == 0) cout << "Scanned " << lineCount << " lines...\r" << flush;
    }
    infile.close();
    cout << "\nTotal IDs found: " << allIDs.size() << endl;

    // Sort Alphabetically
    cout << "Sorting IDs..." << endl;
    sort(allIDs.begin(), allIDs.end());

    // Write to Map File
    cout << "Writing id_map.txt..." << endl;
    ofstream outfile(OUTPUT_FILE);
    if (!outfile) {
        cerr << "Error: Could not create " << OUTPUT_FILE << endl;
        return 1;
    }

    for (size_t i = 0; i < allIDs.size(); ++i) {
        // Format: StringID IntID
        outfile << allIDs[i] << " " << i << "\n";
    }
    outfile.close();

    cout << "Success! Generated map with " << allIDs.size() << " entries." << endl;
    return 0;
}