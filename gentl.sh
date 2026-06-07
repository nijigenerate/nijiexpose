mkdir -p out/

set -- tl/*.po
if [ ! -e "$1" ]; then
    echo "No translations, skipping..."
    exit 0
fi

for f do
    msgfmt -o "out/$(basename -- "$f" .po).mo" -- "$f"
done
