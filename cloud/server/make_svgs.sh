for file in ./*dot
do
 dot $file -Tsvg -o $(basename "$file" .dot).svg
done
