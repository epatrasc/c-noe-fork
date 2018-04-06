echo off
mkdir exec
echo "Starting compiling: src\*.c"
for %%f in (.\src\*.c) do (
    echo "compiling : "%%~nf && gcc ./src/%%~nf.c -o ./exec/%%~nf.exe
    echo "command: gcc ./src/%%~nf.c -o ./exec/%%~nf.exe"
)
echo "Compiling finished"