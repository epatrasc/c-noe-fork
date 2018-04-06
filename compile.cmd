echo off
mkdir exec
echo "Starting compiling: src\*.c"
for %%f in ("src\*.c") do (
    set file=%%~nf
    echo "compiling : "%file% && gcc ./src/%file%.c -o ./exec/%file%.exe
    echo "command: gcc ./src/%file%.c -o ./exec/%file%.exe"
)
echo "Compiling finished"