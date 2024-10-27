#!/bin/bash

LIB_NAME=ac_detour
LIB_PATH=$(pwd)/../Bin/$LIB_NAME.so
PROCID=$(pgrep native_client | head -n 1)

# Check if gdb is installed
if ! command -v gdb &> /dev/null
then
    echo "GDB is not installed"
    exit 1
fi

# Verify permissions
if [[ "$EUID" -ne 0 ]]; then
    echo "Please run as root"
    exit 1
fi

# Check if library exists
if [ ! -f "$LIB_PATH" ]; then
    echo "Library $LIB_PATH not found"
    exit 1
fi

# Check if library is already loaded
if lsof -p $PROCID 2> /dev/null | grep -q '$LIB_PATH'; then
    echo "Library is already loaded"
    exit 1
fi

# Check if the process is running
if [ -z "$PROCID" ]; then
    echo "Process is not running"
    exit 1
fi

unload() {
    echo -e "\nUnloading library..."

    if [ -z "$LIB_HANDLE" ]; then
        echo "Handle not found"
        exit 1
    fi

    gdb -n --batch -ex "attach $PROCID" \
                   -ex "call dlclose($LIB_HANDLE)" \
                   -ex "detach" > /dev/null 2>&1

    echo "Library has been unloaded!"
}

if [ "$#" -gt 0 ]; then
    LIB_HANDLE=$(gdb -n --batch -ex "attach $PROCID" \
                                -ex "set \$detour_handler = dlopen(\"$LIB_PATH\", 1)" \
                                -ex "print/x \$detour_handler" \
                                -ex "source injector_gdb_setup.py" | grep -oP '\$1 = \K0x[0-9a-f]+')

    gdb -ex "attach $PROCID" \
        -ex "call init()" \
        -ex "printf \"\nYou are running this injection script in interactive mode.\nBy default it will open hook resources and call 'init()' constructor.\nWhen you run 'Quit' shared library will be closed by script.\n\n\""

    unload
    exit 0
else
    trap unload SIGINT
    LIB_HANDLE=$(gdb -n --batch -ex "attach $PROCID" \
                                -ex "set \$detour_handler = dlopen(\"$LIB_PATH\", 1)" \
                                -ex "print/x \$detour_handler" \
                                -ex "source injector_gdb_setup.py" \
                                -ex "call init()" \
                                -ex "detach" | grep -oP '\$1 = \K0x[0-9a-f]+')
fi

if [ -z "$LIB_HANDLE" ]; then
    echo "Failed to load library"
    exit 1
fi

echo "Library loaded successfully at $LIB_HANDLE. Use Ctrl+C to unload."

# Create the file if it doesn't exist
if [ ! -e "./$LIB_NAME.log" ]; then
    touch "./$LIB_NAME.log"
fi

tail -f ./$LIB_NAME.log
