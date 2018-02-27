#!/bin/bash

rm -rf cscope.files myproject.db cscope.out tags

find . -iname "*.c"    > ./cscope.files
find . -iname "*.cpp" >> ./cscope.files
find . -iname "*.cxx" >> ./cscope.files
find . -iname "*.cc " >> ./cscope.files
find . -iname "*.h"   >> ./cscope.files
find . -iname "*.hpp" >> ./cscope.files
find . -iname "*.hxx" >> ./cscope.files
find . -iname "*.hh " >> ./cscope.files

echo "Generating cscope db..."
cscope -cbk

echo "Generating ctags db..."
ctags --fields=+i -n -R -L ./cscope.files

echo "Generating code query db.."
cqmakedb -s ./myproject.db -c ./cscope.out -t ./tags -p

