@echo off
if not exist out mkdir out
javac -d out @sources.txt
if errorlevel 1 exit /b 1
java -cp out Main
