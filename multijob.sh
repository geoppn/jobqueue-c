for file in "$@"; do
    while IFS= read -r line; do
        ./jobCommander issueJob "$line"
    done < "$file"
done