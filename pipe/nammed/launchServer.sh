#! /bin/bash

export thePath=$PWD
folder=`basename "$PWD"`
echo
echo '[Launcher - Server]: "'${folder}'" project.'
echo

echo "[Server] launched."
echo

./server.exe

