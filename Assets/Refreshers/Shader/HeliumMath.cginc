#ifndef HELIUM_MATH
#define HELIUM_MATH



float3 ComputeBinormal(float3 n, float3 t, float sign){
    return cross(n,t) * (sign * unity_WorldTransformParams.w/*Handles cases where object is mirrored in some dimension*/);
}

// Leverage simple box rebounding to compute the actual point in the cubemap to sample.
float3 BoxProjectionIfActive(
    // if unprocessed, this direction would sample somewhere completely different
    float3 refelctionDir,
    // This is the point being rendered, aka the point from where the sampling should happen
    float3 fragmentPos,
    // Position of where the cubemap is baked
    float4 cubemapPos, 
    // bounds
    float3 boxMin, 
    float3 boxMax
){
    // Forces branch in the compiled shader, otherwise some implementations
    // might use double computation and step based on the condition to choose the final value.
    UNITY_BRANCH 
    if(cubemapPos.w > 0 ){// Box projection is enabled
        // This is the same math behind the interior mapping technique
        // Bounds relative to the point reflecting light
        boxMin -= fragmentPos;
        boxMax -= fragmentPos;
        // In an axis aligned cube the intersection is easily computed by finding 
        // the closest plane to the source of a ray. 
        float3 intersection;
        intersection.x = (refelctionDir.x > 0? boxMax.x : boxMin.x) / refelctionDir.x;
        intersection.y = (refelctionDir.y > 0? boxMax.y : boxMin.y) / refelctionDir.y;
        intersection.z = (refelctionDir.z > 0? boxMax.z : boxMin.z) / refelctionDir.z;
        // Find the multiplier to go from reflection direction to the vector going from 
        // reflection point (fragment) to the point in the cube being reflected (sampled).
        float multiplier = min(intersection.x, min(intersection.y, intersection.z));
        // Compute the reflected point in the space relative to the cubemap position
        // Equal to the vector going from cubemap to fragment + the vector from fragment to reflected point.
        refelctionDir =  refelctionDir*multiplier + (fragmentPos - cubemapPos);
    }
    return refelctionDir;
}

#endif