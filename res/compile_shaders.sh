#! /bin/bash

# flags
is_forcing=false

# glslang params
shader_include_folders="shaders_include"
vulkan_version=450
preamble_text='--preamble-text '

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

    if [ -f $file_path_dst ]; then
        file_last_modified_dst=$(stat -c %Y $file_path_dst)
    else
        file_last_modified_dst=0 # file was never modified
    fi

    
    if ! $is_forcing && [ $file_last_modified_src -le $file_last_modified_dst ];
    then
        continue
    fi

    glslang -I$shader_include_folders -V --glsl-version $vulkan_version -P"#extension GL_ARB_shading_language_include : require" $file_path_src -o $file_path_dst

done