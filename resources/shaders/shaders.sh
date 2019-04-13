echo -n "Compiling shaders"

mkdir -c build

for filename in ./*.glsl; do
	./glsl-compiler "${filename%.*}" 
done

echo -n "Finish compiling shaders"
echo -n ""
