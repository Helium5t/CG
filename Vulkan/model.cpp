#include "main.h"
#ifdef HELIUM_LOAD_MODEL

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

std::vector<Vert> vertices;
std::vector<uint32_t> indices;

void HelloTriangleApplication::loadModel(){
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
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
            Vert v{};
            indices.push_back(indices.size());
            v.pos = {
                attributes.vertices[3 * i.vertex_index],
                attributes.vertices[3 * i.vertex_index + 1],
                attributes.vertices[3 * i.vertex_index + 2],
            };
            v.texCoords = {
                attributes.texcoords[2 * i.texcoord_index + 0],
                attributes.texcoords[2 * i.texcoord_index + 1]
            };
            v.col = {1.0f, 1.0f, 1.0f};
            vertices.push_back(v);
            pushed++;
        }
    }
    std::cout<< "added "<< pushed << " vertices: " << vertices.size() << std::endl;

}
#endif