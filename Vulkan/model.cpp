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
    std::unordered_map<int, std::unordered_map<int,int>> indexToUVToVertCache;
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
            /*
            A bit less readable but much faster. 
            Compared to having an unordered_map<Vert, int>. 
            First of all, instead of having to compare identity with 5 float comparisons (v.x,v.y.v.z , v.uv.x, v.uv.y), 
            we only need to do 2 integer comparisons (vertex_index, texcoord_index).
            Moreover, if we use Vert as key we are forced to allocate memory for all the indices we receive. Which at load time 
            is the same as just not deduplicating the vertices (on the CPU side that is). 
            Using the indices' two indicies allows us to avoid allocation if the vertex has already been created once. :)
            if n = number of distinct vertex_index and texcoord_index pairs, k = number of vertices.
            A unordered_map<Vert, int> 
                yields a time complexity of O(2n) // Always allocate per each vertex (O(1) * n), then check map access (O(1) *n)
            B std::unordered_map<int,int>
                yields a time complexity of O(k) // Check map (O(1)*n) and if absent allocate (O(1) * k since it's only for absent vertices)
            considering K is at most 1/2 of N (usually much less given how many tris share a vertex with the same UVs)
            O(B) = 3/4 O(A)
            In the example model the number of vertices is a sixth of the indices => k = n/6
            O(B) = O(n) + O(k) = O(n) + O(n/6) = O(7/6n) = 7/12 O(A) => almost 50% faster
            */
            if (indexToUVToVertCache.contains(i.vertex_index)){
                std::unordered_map<int,int> UVToVert = indexToUVToVertCache[i.vertex_index];
                if (UVToVert.contains(i.texcoord_index)){
                    int vertIndex = UVToVert[i.texcoord_index];
                    indices.push_back(vertIndex);
                    continue;
                }
            } 
            Vert v{};
            indexToUVToVertCache[i.vertex_index][i.texcoord_index] = vertices.size(); 
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
    std::cout<< "added "<< pushed << " vertices: " << vertices.size() << std::endl;

}
#endif