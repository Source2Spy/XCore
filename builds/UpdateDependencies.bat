
cd %0/..
echo getting all dependencies

rmdir "../dependencies/freetype" /S /Q
git clone https://github.com/ubawurinna/freetype-windows-binaries.git "../dependencies/freetype"

rmdir "../dependencies/glfw-3.3.bin.WIN64" /S /Q
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('glfw-3.3.bin.WIN64.zip', '../dependencies'); }"

rmdir "../dependencies/properties" /S /Q
git clone https://gitlab.com/LIONant/properties.git "../dependencies/properties"

rmdir "../dependencies/span" /S /Q
git clone https://github.com/tcbrindle/span.git "../dependencies/span"

rmdir "../dependencies/meow_hash_cpp" /S /Q
git clone https://github.com/RedSpah/meow_hash_cpp "../dependencies/meow_hash_cpp"

rmdir "../dependencies/tracy" /S /Q
git clone https://bitbucket.org/wolfpld/tracy/src/master/ "../dependencies/tracy"
cd "../dependencies/tracy"
git checkout 5e8b2a0
cd %0/..

rmdir "../dependencies/freetype" /S /Q
git clone https://github.com/ubawurinna/freetype-windows-binaries.git "../dependencies/freetype"

rem This may be needed at some point but right now we are not looking for too high speed of Compression.
rem A better ratio with still fast decompression is prefer.
rem rmdir "../dependencies/lz4" /S /Q
rem git clone https://github.com/lz4/lz4.git "../dependencies/lz4"

rmdir "../dependencies/zstd" /S /Q
git clone https://github.com/facebook/zstd.git "../dependencies/zstd"

pause