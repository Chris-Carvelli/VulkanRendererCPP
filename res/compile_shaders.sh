#! /bin/bash

path_src="shaders_src"
path_dst="shaders"

for file in "$path_src"/*
do 
    file_name_src=$(basename ${file})
    file_name_dst="$file_name_src.spv"

    # # not needed ATM, but it will be useful for more refined logic
    # shader_name="${file_name%.*}"
    # shader_stage="${file_name##*.}"

    glslang -V $file -o "$path_dst/$file_name_dst"
    #echo "glslang -V $file -o '$path_dst/$file_name_dst'"

done
