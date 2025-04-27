#define STB_IMAGE_IMPLEMENTATION
#include "main.h"

stbi_uc* HelloTriangleApplication::loadImage(const char* path, int* width, int* height, int* channels){
    int a,b,c;
    stbi_uc* pixels = stbi_load(path, width, height, channels, STBI_rgb_alpha);
    return pixels;
}