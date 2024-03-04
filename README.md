# Veeder-Root Automatic Tank Gauge Client

This is a C implementation of my previous client code in [Veeder-Root TLS Socket Library](https://github.com/eredden/Veeder-Root-TLS-Socket-Library), used for interacting with TLS-3xx and TLS-4xx automatic tank gauges. Created primarily for learning purposes and not intended as a successor to my original project, which contains more extensive error-handling currently.

You can find commands to interact with the automatic tank gauges in the [VEEDER - ROOT SERIAL INTERFACE MANUAL for TLS-300 and TLS-350 UST Monitoring Systems and TLS-350R Environmental & Inventory Management System](https://cdn.chipkin.com/files/liz/576013-635.pdf) manual.

## Usage

Compile `veeder-client.c` in the `src` directory using your compiler of choice -- I personally use `gcc`. Execute the newly made executable with three arguments -- the hostname or IP address of the automatic tank gauge, the port number (generally 10001), and the timeout to wait for responses.

```
me@computer:~/Downloads$ ./veeder-client 127.0.0.1 NAME 10001 1
Attempting to establish connection...
Connected to 127.0.0.1 on port 10001!

> i10100
i101002403031036211901&&FB34
> exit
Terminating connection...
```

You can see in the above snippet of me running the program that I type in the serial command to interact with the automatic tank gauge, and an unfiltered response is returned. Then, I use `exit` to terminate the connection and end the program.

## To-Do

This is a list of things I intend on changing or fixing in the program.

- Validate server response using checksum at end of message.