# Aether: High-Performance C++ Search Engine

![Aether UI](https://via.placeholder.com/800x400?text=Aether+Search+UI)

**Aether** is a next-generation decentralized academic search engine designed for speed and scalability. It leverages a high-performance **C++ Core** for indexing and retrieval, coupled with a modern **React (Vite)** frontend for a seamless user experience.

## 🚀 Architecture

```mermaid
graph TD
    User[User] -->|Interact| React[React Frontend (Port 5173)]
    React -->|HTTP Requests| FastAPI[FastAPI Backend (Port 8000)]
    FastAPI -->|Subprocess| CPP[C++ Search Engine Core]
    
    subgraph "Core Engine"
        CPP -->|Reads| RAM[In-Memory Lexicon & Trie]
        CPP -->|Seeks (O(1))| HDD[Barrel Files (On Disk)]
        CPP -->|Ranks| PageRank[PageRank Graph]
    end
```

## ✨ Key Features

-   **Hybrid Indexing:** Inverted Index (Barrels) + Forward Index for rapid retrieval.
-   **Hot-Swap Architecture:** Update the dataset in real-time (Double Buffering) without downtime.
-   **Trie-Based Autocomplete:** Instant search suggestions as you type.
-   **Advanced Ranking:** Custom BM25 algorithm combined with PageRank centrality scores.
-   **Nebula UI:** A polished, animated interface using Glassmorphism and Framer Motion.
-   **Smart Queries:** Supports boolean logic, date sorting (`/date`), and category filtering (`/cat:cs.AI`).

## 🛠️ Setup & Installation

### Prerequisites
-   **C++ Compiler** (g++ with C++17 support)
-   **Python 3.8+**
-   **Node.js & npm**

### Quick Start
We provide a unified setup script to compile the backend and install frontend dependencies.

```powershell
./setup_project.bat
```

### Manual Build
**Backend:**
```bash
pip install fastapi uvicorn flask-cors
g++ -O3 -std=c++17 searchengine.cpp -o searchengine.exe
g++ -O3 -std=c++17 trie_builder.cpp -o trie_builder.exe
```

**Frontend:**
```bash
cd frontend
npm install
npm run dev
```

## 🌐 Deployment (Remote Access)
To expose your local instance to the world:
1.  Run the backend locally: `uvicorn main:app --host 0.0.0.0 --port 8000`
2.  Use **Cloudflare Tunnel** (Recommended) or **ngrok** to tunnel traffic to port 8000.
3.  Set the switch in the UI to **"Server: Cloud"** or let "Auto" detect the latency.

## 📄 License
MIT License. Built by [Your Name] & [Partner Name].
