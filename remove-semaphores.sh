#!/bin/bash

# Get all semaphore IDs and remove them
for semid in $(ipcs -s | awk '{print $2}' | tail -n +4); do
    ipcrm -s $semid
done