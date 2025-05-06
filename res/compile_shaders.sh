#! /bin/bash

# flags
is_forcing=false

while getopts 'f' flag; do
  case "${flag}" in
    f) is_forcing=true ;;
  esac
done

path_src="shaders_src"
path_dst="shaders"

for file in "$path_src"/*
do 
    file_name_src=$(basename ${file})
    file_name_dst="$file_name_src.spv"

    file_path_src=$file
    file_path_dst="$path_dst/$file_name_dst"

    file_last_modified_src=$(stat -c %Y $file_path_src)
    file_last_modified_dst=$(stat -c %Y $file_path_dst)

    
    if ! $is_forcing && [ $file_last_modified_src -le $file_last_modified_dst ];
    then
        continue
    fi
    # # not needed ATM, but it will be useful for more refined logic
    # shader_name="${file_name%.*}"
    # shader_stage="${file_name##*.}"

    glslang -I$path_src/include -V $file_path_src -o $file_path_dst
    #echo "glslang -V $file -o '$path_dst/$file_name_dst'"

done
