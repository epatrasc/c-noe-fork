mkdir exec
for entry in src/*; do
    export file=$(echo $(basename "$entry") | cut -f 1 -d '.')
    echo "command: : gcc ./src/$file.c -o ./exec/$file.exe"
    echo "compiling : "$file && gcc ./src/$file.c -o ./exec/$file.exe
done