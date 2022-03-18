#!/bin/bash

SPEED=1
DELAY=1
LOSS=0
CORRUPT=100
CORRUPT2=0
REORDER=0

{
    killall link
    killall recv
    killall send
} &> /dev/null

./link_emulator/link speed=$SPEED delay=$DELAY loss=$LOSS corrupt=$CORRUPT corrupt2=$CORRUPT2 reorder=$REORDER &
sleep 1
./recv &
sleep 1

./send
