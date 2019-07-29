

echo getting all dependencies

rmdir "../src/dependencies/properties" /S /Q
git clone https://gitlab.com/LIONant/properties.git "../dependencies/properties"

rmdir "../src/dependencies/span" /S /Q
git clone https://github.com/tcbrindle/span.git "../dependencies/span"

rmdir "../src/dependencies/meow_hash_cpp" /S /Q
git clone https://github.com/RedSpah/meow_hash_cpp "../dependencies/meow_hash_cpp"


pause