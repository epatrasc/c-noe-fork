for entry in source/*; do
    export file=$(echo $(basename "$entry") | cut -f 1 -d '.')
    echo "compiling : "$file && gcc ./source/$file.c -o ./exec/$file.exe
    #echo "compiling, stop to assembly: "$file && gcc -S ./source/$file.c -o ./assembly/$file.s
done