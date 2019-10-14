echo -n "Compiling shaders"

mkdir -c build

for filename in ./*.glsl ./*.comp ./*.ray; do
	./glsl-compiler "${filename}" 
done

echo -n "Finish compiling shaders"
echo -n ""
