#include "main.h"
#ifdef HELIUM_LOAD_MODEL

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

std::vector<Vert> vertices;
std::vector<uint32_t> indices;

void HelloTriangleApplication::loadModel(){
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::unordered_map<int, int> indexToVertCache;
    std::string msgErr;

    if(!tinyobj::LoadObj( // LoadObj automatically transforms all n-gons to tris. 
        &attributes, 
        &shapes,
        &materials,
        &msgErr,
        MODEL_PATH.c_str()
    )){
        throw std::runtime_error("failed to load obj:" + msgErr);
    }
    int pushed = 0;
    for (const auto& s : shapes){
        for(const auto& i : s.mesh.indices){
            if (indexToVertCache.contains(i.vertex_index)){
                int vertIndex = indexToVertCache[i.vertex_index]; // index of the cached vertex in the array
                indices.push_back(vertIndex);
            }else{
                Vert v{};
                indexToVertCache[i.vertex_index] = vertices.size(); 
                indices.push_back(vertices.size());
                v.pos = {
                    attributes.vertices[3 * i.vertex_index],        //x
                    attributes.vertices[3 * i.vertex_index + 1],    //y
                    attributes.vertices[3 * i.vertex_index + 2],    //z
                };
                v.texCoords = {
                    attributes.texcoords[2 * i.texcoord_index + 0],
                    1.0f - attributes.texcoords[2 * i.texcoord_index + 1]
                };
                v.col = {1.0f, 1.0f, 1.0f};
                vertices.push_back(v);
                pushed++;
            }

        }
    }
    std::cout<< "added "<< pushed << " vertices: " << vertices.size() << std::endl;

}
#endif