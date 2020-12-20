#!/bin/bash

#rm -rf file_*

#dd if=/dev/zero of=file_10m bs=1M count=10
#dd if=/dev/zero of=file_10m bs=1M count=500
#dd if=/dev/zero of=file_10m bs=1G count=1


if [ ! -d "./file/" ];then
        mkdir file
fi

if [ ! -f "./file/file_1.txt" ];then
        for i in {1..1000000}
        do
                echo "11111111111111111111" >> "./file/file_1.txt";
        done
fi

if [ ! -f "./file/file_2.txt" ];then
        for i in {1..1000000}
        do
                echo "22222222222222222222" >> "./file/file_2.txt";
        done
fi

echo -e "\n\nfiles:\n"
ls -lh file/*
echo -e "\n\n"

./main
