
#ifndef HELIUM_MAT_MACROS_INCLUDED
#define HELIUM_MAT_MACROS_INCLUDED

#if !defined(HELIUM_ADD_PASS) && defined(HELIUM_AMBIENT_OCCLUSION)
    #ifdef HELIUM_OCCLUSION_FROM_MAP
        #define OCCLUSION(uv) lerp(1 ,tex2D(_Occlusion, uv.xy), _OcclusionStrength)
    #else
        #define OCCLUSION(uv) 1
    #endif
#else
    #define OCCLUSION(uv) 1
#endif


#ifndef HELIUM_R_FROM_ALBEDO
    #define ALPHA(uv) UNITY_ACCESS_INSTANCED_PROP(_Color_arr, _Color).a * tex2D(_MainTex, uv.xy).a
#else
    #define ALPHA(uv) UNITY_ACCESS_INSTANCED_PROP(_Color_arr, _Color).a
#endif


#ifdef HELIUM_FOG_ACTIVE
    #ifdef HELIUM_ADD_PASS
        #define FOG_COLOR 0.0
    #else
        #define FOG_COLOR unity_FogColor.rgb
    #endif

    #ifdef HELIUM_FOG_USE_CLIP_SPACE_DEPTH
        #define HELIUM_COMPUTE_FOG(c, vo)\
        float viewDist =  UNITY_Z_0_FAR_FROM_CLIPSPACE(vo.pos.z*vo.pos.w);\
        UNITY_CALC_FOG_FACTOR_RAW(viewDist);\
        c.rgb = lerp(FOG_COLOR, c.rgb, saturate(unityFogFactor));
    #else
        #define HELIUM_COMPUTE_FOG(c, vo)\
        float viewDist = length(_WorldSpaceCameraPos - vo.wPos);\
        UNITY_CALC_FOG_FACTOR_RAW(viewDist);\
        c.rgb = lerp(FOG_COLOR, c.rgb, saturate(unityFogFactor));
    #endif
    
#else
    #define HELIUM_COMPUTE_FOG(c, vo);
#endif 


#define HELIUM_COMPUTE_REFLECTION(pn, a, b,c, destName) \
float3 rsv##pn = reflect(-a, b.n);\
rsv##pn = BoxProjectionIfActive(rsv##pn, b.wPos, unity_SpecCube##pn##_ProbePosition, unity_SpecCube##pn##_BoxMin, unity_SpecCube##pn##_BoxMax); \
/* Since they are all cubemaps of the same resolution etc..., 
    all specular probe cubemaps use the sampler from unity_SpecCube0 */ \
float4 specHDR##pn = UNITY_SAMPLE_TEXCUBE_SAMPLER_LOD(unity_SpecCube##pn , unity_SpecCube0, rsv##pn, c * ROUGHNESS(b.uvM.xy) * HELIUM_MAX_LOD); \
float3 destName = DecodeHDR(specHDR##pn, unity_SpecCube##pn##_HDR); 



#ifdef DOEONIDOI
#else

    #define HELIUM_HEURISTIC_PARALLAX_BIAS 0.42

    #define PARALLAX(uv, tanSpaceDir, strength) \
    tanSpaceDir = normalize(tanSpaceDir); \
    tanSpaceDir.xy /= tanSpaceDir.z + HELIUM_HEURISTIC_PARALLAX_BIAS; /* Corrects xy length to see where the ray will reach when fully extended (plus an heuristic correction taken from Unity itself)*/ \
    uv += tanSpaceDir.xy * (strength);
#endif

#endif