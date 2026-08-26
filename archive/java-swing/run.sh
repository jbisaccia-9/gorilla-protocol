#!/bin/sh
mkdir -p out
javac -d out @sources.txt
java -cp out Main
