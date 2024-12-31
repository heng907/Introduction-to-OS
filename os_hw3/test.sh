#!/bin/bash

python3 testcase.py
echo "Testcase generated"
g++ -pthread main.cpp
echo "Compiled"
echo ""
./a.out