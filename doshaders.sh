#!/bin/bash

# compiles all GLSL files if 0 args and enters yes
if [[ $# == 0 ]]; then
    
    read -p "Compile all shaders? (y/N) " COMPILE
    
    if [[ $COMPILE == "y" || $COMPILE == "Y" ]]; then
        echo "Compiling all shaders"
        FAILURE=0

        cd source/res/shaders

        glslc -fshader-stage=fragment ../../shaders/*.frag -c
    
        if [[ $? -ne 0 ]];
        then
            echo "Failed to compile all shaders"
            FAILURE=1
        fi

        glslc -fshader-stage=vertex ../../shaders/*.vert -c

        if [[ $? -ne 0 ]];
        then
            echo "Failed to compile all shaders"
            FAILURE=1
        fi

        if [[ $FAILURE -eq 1 ]]; then
            echo "Some files failed to compile"
            exit 1
        fi

        echo "done."
        
        cd ../..
    fi
elif [[ $# -eq 2 && $1 -eq 'f' && -f source/shaders/$2 ]]; then

    glslc -fshader-stage=fragment source/shaders/$2 -o source/res/shaders/$2.spv

elif [[ $# -eq 2 && $1 -eq 'v' && -f source/shaders/$2 ]]; then

    glslc -fshader-stage=vertex source/shaders/$2 -o ./source/res/shaders/$2.spv

fi