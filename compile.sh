#!/bin/bash

mkdir exec
gcc -c ./src/library.c -o ./exec/library.o
for entry in src/*.c; do
    export file=$(echo $(basename "$entry") | cut -f 1 -d '.')
    if test "$file" = 'library'; then
        continue
    fi
    echo "command: : gcc ./src/$file.c ./exec/library.o -o ./exec/$file.exe"
    echo "compiling : "$file && gcc ./src/$file.c ./exec/library.o -o ./exec/$file.exe
done