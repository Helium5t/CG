
# Prerequisites #
I develop on MacOs and hate XCode, so this readme assumes an Apple Silicon environment and VSCode as IDE.
 - (Vulkan SDK)[https://vulkan.lunarg.com/sdk/home] (Follow (Vulkan Setup)[./#Vulkan%20Setup] for more)
 - VSCode 
 - Clang (comes in built-in with macOS)
 - XCode (`xcode-select --install` otherwise)
 - Cmake (`brew install cmake` otherwise)
 - Ninja (`brew install ninja` otherwise)
 - wget (`brew install wget` otherwise)

# Setup #
Assuming you have everything setup, all necessary components should be ready to go, you can proceed to build either via VSCode or via:
```cmake --build ./build --config {Debug|Release} --target all -j {number of parallel processes}```

You can then run with the following command:
```./build/hello`


## Vulkan Setup ##
After installing vulkan. Make sure the Vulkan environment variables are set. If this is the first setup they probably aren't, so you can run the following set of lines.
<!---TODO: make bash script--->
```
export VULKAN_SDK={Path to vulkan sdk}/macOS
export PATH=$VULKAN_SDK/bin:$PATH
export DYLD_LIBRARY_PATH=$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH
export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
export VK_ADD_LAYER_PATH="$VULKAN_SDK/share/vulkan/explicit_layer.d"
export VK_DRIVER_FILES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
export PKG_CONFIG_PATH="$VULKAN_SDK/lib/pkgconfig:$PKG_CONFIG_PATH"
```


### Useful defines ###
- HELIUM_PRINT_PHYS_EXT : Prints supported extensions for physical device
- HELIUM_PRINT_EXTENSIONS : Prints supported instance extensions
- HELIUM_PRINT_LAYERS : Prints available layers
- HELIUM_DEBUG_LOG_FRAMES: Printing for each frame can slow down things, so define this when you need debug prints inside the frame rendering process (drawFrame()).
- HELIUM_DO_NOT_REFRESH : Some platforms crash after the first frame. This is due to some moltenVK incompatibility (same code with no changes works on other platforms, both mac and not )