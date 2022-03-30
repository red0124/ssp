#!/bin/bash

set -x 
set -e

python3 script/single_header_generator.py > ssp.cpp

echo "" >> ssh.cpp
echo 'int main(){ ss::parser p{""}; p.get_next<int, float>(); return 0; }' \
    >> ssp.cpp

g++ -std=c++17 ssp.cpp -o ssp.bin -Wall -Wextra
./ssp.bin

rm ssp.cpp ssp.bin
