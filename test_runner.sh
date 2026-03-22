#!/bin/bash

PROBABILITIES=("0.05" "0.10" "0.15" "0.20" "0.25" "0.30" "0.35" "0.40" "0.45" "0.50")

echo "Starting Automated KTP Testing..."

for p in "${PROBABILITIES[@]}"
do
    echo "[*] Testing with DROP_PROB = $p"
    
    sed -i "s/#define DROP_PROB .*/#define DROP_PROB $p/" ksocket.h
    
    gcc -c ksocket.c -o ksocket.o
    ar rcs libksocket.a ksocket.o
    gcc initksocket.c -o initksocket -L. -lksocket -lpthread
    gcc user1.c -o user1 -L. -lksocket -lpthread
    gcc user2.c -o user2 -L. -lksocket -lpthread

    ./initksocket > /dev/null 2>&1 &
    INIT_PID=$!
    sleep 1 

    ./user2 > /dev/null 2>&1 &
    USER2_PID=$!
    sleep 1 

    ./user1 | grep -E "Average Transmissions|Total Unique|Total Transmissions"
    
    sleep 2
    kill -9 $INIT_PID 2>/dev/null
    kill -9 $USER2_PID 2>/dev/null
    
    ipcrm -M 100 2>/dev/null 
    
done
