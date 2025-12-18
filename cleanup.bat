@echo off
echo ============================================
echo   CLEANING PROJECT FOR GITHUB / TRANSFER
echo ============================================

echo Deleting venv...
if exist venv rmdir /s /q venv

echo Deleting node_modules (this may take a while)...
if exist frontend\node_modules rmdir /s /q frontend\node_modules
if exist frontend\dist rmdir /s /q frontend\dist

echo Deleting static assets...
if exist static rmdir /s /q static

echo Deleting compiled binaries...
del /q *.exe 2>nul
del /q *.o 2>nul
del /q *.obj 2>nul

echo Deleting Python cache...
for /d /r . %%d in (__pycache__) do @if exist "%%d" rmdir /s /q "%%d"

echo Deleting temp scripts...
del /q fix_*.bat 2>nul
del /q rebuild_*.bat 2>nul
:: Keep setup_project.bat and run_aether.bat

echo ============================================
echo   CLEANUP COMPLETE
echo   Ready to zip or git push!
echo ============================================
pause
