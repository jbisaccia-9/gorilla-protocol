# Java Swing Prototype

This directory preserves the original desktop prototype of Gorilla Golden Eye.
It is a historical snapshot; active development and the live cloud release are
in the repository root.

## Requirements

- JDK 8 or newer
- A desktop environment capable of opening a Swing window

## Run

From this directory:

```sh
./run.sh
```

On Windows, run `run.bat` instead.

The equivalent manual commands are:

```sh
mkdir -p out
javac -d out @sources.txt
java -cp out Main
```

The generated `out/` directory is ignored by Git.
