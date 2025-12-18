@echo off
set "PATH=%PATH%;C:\Program Files\nodejs"
echo ============================================
echo   AETHER SEARCH ENGINE - SETUP WIZARD
echo ============================================

:: 1. PREREQUISITES CHECK
echo [1/6] Checking Prerequisites...
where python >nul 2>nul
if %errorlevel% neq 0 ( echo ERROR: Python not found. & pause & exit /b )
where node >nul 2>nul
if %errorlevel% neq 0 ( echo ERROR: Node.js not found. & pause & exit /b )
where g++ >nul 2>nul
if %errorlevel% neq 0 ( echo ERROR: g++ not found. & pause & exit /b )
echo OK.

:: 2. PYTHON DEPS
echo [2/6] Installing Python Dependencies...
python -m pip install fastapi uvicorn python-multipart requests
echo OK.

:: 3. COMPILING BACKEND
echo [3/6] Compiling C++ Core...
g++ -O3 -std=c++17 searchengine.cpp -o searchengine.exe
g++ -O3 -std=c++17 trie_builder.cpp -o trie_builder.exe
:: Assuming add_document.cpp exists or we create a dummy for now
if exist add_document.cpp g++ -O3 -std=c++17 add_document.cpp -o add_document.exe
echo OK.

:: 4. FRONTEND SETUP
echo [4/6] Setting up Frontend...
if not exist frontend (
    echo Creating React Project...
    call npx -y create-vite@latest frontend --template react
    cd frontend
    call npm install -D tailwindcss postcss autoprefixer
    call npx tailwindcss init -p
) else (
    cd frontend
)

echo Installing Node Modules...
call npm install
cd ..
echo OK.

:: 5. BUILDING TRIE (If needed)
if not exist trie.bin (
    echo [5/6] Generating Autocomplete Index...
    trie_builder.exe
) else (
    echo [5/6] Autocomplete Index exists. Skipping.
)

:: 6. FINAL BUILD
echo [6/6] Building Frontend Production Bundle...
cd frontend
call npm run build
cd ..
:: Copy dist to static for FastAPI
if exist static rmdir /s /q static
xcopy frontend\dist static /E /I /Y

echo ============================================
echo   SETUP COMPLETE!
echo   Run: uvicorn main:app --reload
echo ============================================
pause