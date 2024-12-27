#! /bin/bash

export thePath=$PWD
folder=`basename "$PWD"`
echo
echo '[Launcher - Client]: "'${folder}'" project.'
echo

echo "[Client] launched."
echo
./client.exe

