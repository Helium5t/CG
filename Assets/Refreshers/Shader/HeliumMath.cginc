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



#define HELIUM_HEURISTIC_PARALLAX_BIAS 0.42

#ifdef HELIUM_PARALLAX_OFFSET
    #define PARALLAX_CORE(uv, heightMap, s, tsd, df)\
        float o =  tsd * (tex2D(heightMap, uv.xy).r - 0.5) * s;;\
        uv.xy += o;\ 
        uv.zw += o * df;
#elif defined(HELIUM_PARALLAX_RAYMARCH)
    #ifndef HELIUM_PARALLAX_RAYMARCHING_STEPS 
        #define HELIUM_PARALLAX_RAYMARCHING_STEPS  10
    #endif
    void RaymarchedParallax(inout float4 uv, inout float3 tsd, sampler2D h, float s, float2 df){
        float2 uvOffset = 0;
        float ss = 1.0/HELIUM_PARALLAX_RAYMARCHING_STEPS;
        float2 uvStep = tsd * (ss * s);
        float sh = 1;
        float surfaceHeight = tex2D(h, uv);
        for (int i = 1; i < HELIUM_PARALLAX_RAYMARCHING_STEPS && sh > surfaceHeight; i++){
            uvOffset -= uvStep;
            sh -= ss;
            surfaceHeight = tex2D(h, uv + uvOffset);
        }
        uv.xy += uvOffset;
        uv.zw += uvOffset * df;
    }
    #define PARALLAX_CORE(uv, height, s, tsd, df) RaymarchedParallax(uv, tsd, height, s, df)
#else
    #define PARALLAX_CORE(_,__,___,____,_____) return
#endif


void DisplaceUVParallax(inout float4 uv, inout float3 tsd, sampler2D h, float s, float2 detailFactor){
    tsd = normalize(tsd);
    tsd.xy/=tsd.z + HELIUM_HEURISTIC_PARALLAX_BIAS;
    PARALLAX_CORE(uv, h, s, tsd, detailFactor);
}

#endif