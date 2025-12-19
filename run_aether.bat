@echo off
echo Creating Virtual Environment...
python -m venv venv
echo Installing Dependencies in Venv...
.\venv\Scripts\python -m pip install flask flask-cors requests
echo Building C++ Tools...
g++ -O3 -std=c++17 add_document.cpp -o add_document.exe
g++ -O3 -std=c++17 invert.cpp -o invert.exe
g++ -O3 -std=c++17 create_barrels.cpp -o create_barrels.exe
g++ -O3 -std=c++17 searchengine.cpp -o searchengine.exe

echo Building Frontend...
cd frontend
call npm run build
cd ..
echo Deploying Frontend...
if exist static rmdir /s /q static
xcopy frontend\dist static /E /I /Y >nul
echo Starting Aether...
.\venv\Scripts\python main.py
pause