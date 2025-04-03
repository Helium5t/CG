#!/bin/zsh
set -e
# Check if the correct number of arguments are passed
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <vertexShaderPath> <fragmentShaderPath>"
  exit 1
fi

vertPath="$1"
fragPath="$2"


outNameVert="${1%.*}"

outNameFrag="${2%.*}"

echo "Compiling shaders using: $VULKAN_SDK/bin/glslc"

$VULKAN_SDK/bin/glslc -fshader-stage=vert "$1" -o "${outNameVert}.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=frag "$2" -o "${outNameFrag}.spv"
