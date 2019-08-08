
cd %0/..
echo getting all dependencies

rmdir "../dependencies/properties" /S /Q
git clone https://gitlab.com/LIONant/properties.git "../dependencies/properties"

rmdir "../dependencies/span" /S /Q
git clone https://github.com/tcbrindle/span.git "../dependencies/span"

rmdir "../dependencies/meow_hash_cpp" /S /Q
git clone https://github.com/RedSpah/meow_hash_cpp "../dependencies/meow_hash_cpp"

rmdir "../dependencies/tracy" /S /Q
git clone https://bitbucket.org/wolfpld/tracy/src/master/ "../dependencies/tracy"

pause