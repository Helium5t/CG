### Useful defines ###
- HELIUM_PRINT_PHYS_EXT : Prints supported extensions for physical device
- HELIUM_PRINT_EXTENSIONS : Prints supported instance extensions
- HELIUM_PRINT_LAYERS : Prints available layers
- HELIUM_DEBUG_LOG_FRAMES: Printing for each frame can slow down things, so define this when you need debug prints inside the frame rendering process (drawFrame()).
- HELIUM_DO_NOT_REFRESH : Some platforms crash after the first frame. This is due to some moltenVK incompatibility (same code with no changes works on other platforms, both mac and not )