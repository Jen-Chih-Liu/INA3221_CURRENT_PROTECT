@echo off
REM 切換到專案目錄


REM 建立 build 資料夾 (如果不存在)
if not exist build mkdir build

REM 進入 build 資料夾
cd build

REM 執行 CMake 配置
cmake ..
if %errorlevel% neq 0 exit /b %errorlevel%

REM 執行編譯
cmake --build .
if %errorlevel% neq 0 exit /b %errorlevel%

REM 複製執行檔到專案根目錄 (搜尋子目錄以相容 Debug/Release 資料夾)
for /r %%f in (FILESUM.exe) do copy "%%f" ..\FILESUM.exe /y

REM 回到上一層
cd ..

REM 刪除 build 資料夾 (包含所有中間產物)
rd /s /q build

echo Build Complete!
pause
