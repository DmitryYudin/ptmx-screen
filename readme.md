# Pseudo-terminal usage example

This code illustrates how to create `master` terminal and attach to it using bash script (`screen`).
This migth be usefull if develop `cli` frontend for the application which writes it's logs directly to `stdin` and `stderr`.

## Build

```
    sudo apt-get install librange-v3-dev libboost-dev
    gcc -o master master.cc -lstdc++ -lboost_regex
```

## Run

```
    ./master
    ./slave.sh
```

On startup, `master` opens `/dev/ptmx` and writes the `slave`-end to `/tmp/master.pts`.
`Slave` in turn finds an end-point and sends `hello` message to allow `master` to report its status.

After this, every message entered by the user on the `slave` side is read and echoed back by the `master` (i.e. appears on a user's terminal).
