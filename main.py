import os
import subprocess
import shutil
import time
from threading import Thread
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS

app = Flask(__name__, static_folder='static')
CORS(app)

# --- CONFIGURATION ---
RAILWAY_ENVIRONMENT = os.getenv("RAILWAY_ENVIRONMENT", "false").lower() == "true"
SEARCH_ENGINE_PATH = "./searchengine.exe" if os.name == 'nt' else "./searchengine"
DOC_LIMIT = "10000" if RAILWAY_ENVIRONMENT else "0"

engine_process = None

# --- PROCESS MANAGEMENT ---
def start_engine():
    global engine_process
    args = [SEARCH_ENGINE_PATH, "--json"]
    if DOC_LIMIT != "0":
        args.extend(["--limit", DOC_LIMIT])
    
    print(f"Starting Search Engine: {' '.join(args)}")
    
    if not os.path.exists(SEARCH_ENGINE_PATH):
        print(f"CRITICAL ERROR: Search engine binary not found at {SEARCH_ENGINE_PATH}")
        print(f"Current Directory Contents: {os.listdir('.')}")
        return

    engine_process = subprocess.Popen(
        args,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

def stop_engine():
    global engine_process
    if engine_process:
        engine_process.stdin.close()
        engine_process.terminate()
        engine_process.wait()

start_engine()

# --- API HELPERS ---
def send_to_engine(command: str) -> str:
    if not engine_process or engine_process.poll() is not None:
        start_engine() # Restart if dead
        time.sleep(1)
    
    try:
        engine_process.stdin.write(command + "\n")
        engine_process.stdin.flush()
        return engine_process.stdout.readline()
    except Exception as e:
        return f'{{"error": "{str(e)}"}}'

# --- ENDPOINTS ---
@app.route("/health")
def health_check():
    status = "online" if engine_process and engine_process.poll() is None else "offline"
    return jsonify({"status": status, "env": "cloud" if RAILWAY_ENVIRONMENT else "local"})

@app.route("/search")
def search():
    q = request.args.get('q', '')
    res_str = send_to_engine(q)
    return res_str, 200, {'Content-Type': 'application/json'}

@app.route("/suggest")
def suggest():
    q = request.args.get('q', '')
    res_str = send_to_engine(f"/suggest {q}")
    return res_str, 200, {'Content-Type': 'application/json'}

import json

# ... (Previous Process Management Code) ...

def run_add_document(file_path):
    print(f"--- Ingesting {file_path} ---")
    
    # METADATA EXTRACTION
    title = "Uploaded Document"
    authors = "System Updater"
    date = "2025-01-01"
    original_id = "upload/new"
    
    try:
        # Load Content
        content = ""
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # 1. Try JSON
        try:
            data = json.loads(content)
            if isinstance(data, dict):
                # TITLE
                if 'title' in data: title = data['title']
                
                # AUTHORS
                if 'authors' in data: 
                    authors = data['authors']
                elif 'submitter' in data:
                    authors = data['submitter']
                    
                # DATE
                if 'update_date' in data:
                    date = data['update_date']
                elif 'versions' in data and len(data['versions']) > 0:
                    date = data['versions'][0].get('created', '2025-01-01')
                    
                # ID
                if 'id' in data:
                    original_id = data['id']
                    
                # Clean up newlines in title/authors to prevent metadata corruption
                title = title.replace('\n', ' ').replace('|', '-')
                authors = authors.replace('\n', ' ').replace('|', '-')
                original_id = original_id.replace('\n', '').replace('|', '-')
        except:
            # Fallback to Text
            if content.strip():
                lines = content.split('\n')
                clean_line = lines[0].strip()[:60]
                if clean_line: title = clean_line

    except Exception as e:
        print(f"Metadata Extraction Failed: {e}")

    try:
        # 1. Add Document -> Pass Metadata
        print(f"Adding: {title} by {authors} ({date}) [{original_id}]")
        
        cmd = [".\\add_document.exe", file_path, title, authors, date, original_id]
        subprocess.run(cmd, check=True)
        
        # 2. Invert
        print("Running Invert...")
        subprocess.run([".\\invert.exe"], check=True)

        # 3. Barrels
        print("Updating Barrels...")
        subprocess.run([".\\create_barrels.exe"], check=True)
        
        print("Restarting Engine...")
        stop_engine()
        start_engine()
        
    except Exception as e:
        print(f"Ingestion Failed: {e}")
    finally:
        if os.path.exists(file_path):
            os.remove(file_path)

@app.route("/upload", methods=['POST'])
def upload_docs():
    file = request.files['file']
    file_location = f"temp_{file.filename}"
    file.save(file_location)
    
    # Background Ingest
    Thread(target=run_add_document, args=(file_location,)).start()
    
    return jsonify({"status": "ingesting", "message": "File accepted. Processing in background..."})

# --- FRONTEND SERVING ---
@app.route("/", defaults={'path': ''})
@app.route("/<path:path>")
def serve(path):
    if path != "" and os.path.exists(app.static_folder + '/' + path):
        return send_from_directory(app.static_folder, path)
    else:
        return send_from_directory(app.static_folder, 'index.html')

if __name__ == "__main__":
    try:
        app.run(host='0.0.0.0', port=8000)
    finally:
        stop_engine()