for file in "$@"; do
    while IFS= read -r line; do
        ./JobCommander "$line"
    done < "$file"
done