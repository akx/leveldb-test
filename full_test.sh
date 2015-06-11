#!/bin/bash
rm -rf testdb
./build.sh
time ./ldbtest write
time ./ldbtest read
