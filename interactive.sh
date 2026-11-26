#!/bin/bash

# for when im too lazy to do a ^ + enter

while [[ 1 -eq 1 ]];
do
    echo "1. compile"
    echo "2. run"
    echo "3. delete build"
    echo "4. compile shaders"
    echo "5. exit"
    read -p "Select: " SELECT

    if [[ $SELECT -eq 1 ]]; then
        ./cnst.sh
    elif [[ $SELECT -eq 2 ]]; then
        ./sprnt.sh
    elif [[ $SELECT -eq 3 ]]; then
        ./ridbuild.sh
    elif [[ $Select -eq 4 ]]; then
        ./doshaders.sh
    else
        exit 0
    fi
done