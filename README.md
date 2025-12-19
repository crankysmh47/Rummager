AETHER SEARCH ENGINE

OVERVIEW
--------
Aether is a hybrid C++/Python search engine with a React frontend. 
It uses custom binary indexing (Barrels) and a Semantic Association Model.

PREREQUISITES
-------------
1. G++ Compiler (MinGW for Windows or standard G++ for Linux)
2. Python 3.9+
3. Node.js (for building the frontend)
4. Cloudflared (Optional, only for public access)

QUICK START (WINDOWS)
---------------------
1. Double-click "run_aether.bat".
   - This script automatically:
     a) Sets up the Python virtual environment.
     b) Compiles all C++ source files.
     c) Builds the React frontend.
     d) Starts the server at http://localhost:8000.

MANUAL BUILD (LINUX/MAC)
------------------------
1. Compile C++:
   g++ -O3 -std=c++17 searchengine.cpp -o searchengine
   g++ -O3 -std=c++17 trie_builder.cpp -o trie_builder
   g++ -O3 -std=c++17 invert.cpp -o invert
   g++ -O3 -std=c++17 create_barrels.cpp -o create_barrels
   g++ -O3 -std=c++17 add_document.cpp -o add_document

2. Frontend:
   cd frontend && npm install && npm run build && cd ..
   mkdir static
   cp -r frontend/dist/* static/

3. Run:
   pip install flask flask-cors
   python main.py

MAKING IT ONLINE (PUBLIC ACCESS)
--------------------------------
To generate a public URL (e.g., https://random-name.trycloudflare.com) that 
allows the teacher to access your locally running project from their device:

1. Ensure the Aether project is already running (Step 1 above).
2. Open a *new* terminal window (do not close the one running the engine).
3. Type the following command:

   cloudflared tunnel --url http://localhost:8000

4. Copy the URL ending in ".trycloudflare.com" from the terminal output.
   Share this URL for public access.

DIRECTORY STRUCTURE
-------------------
/cpp_src    - Low-level indexing and search algorithms
/frontend   - React user interface
main.py     - Python orchestration layer
barrels/    - Binary index files (Generated automatically)
