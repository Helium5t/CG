#ifndef HELIUM_LIGHTING_COMMON
#define HELIUM_LIGHTING_COMMON

// #include "UnityCG.cginc" // Already included by UnityStandardBRDF
// #include "UnityStandardBRDF.cginc" // in UnityPBSLighting
// #include "UnityStandardUtils.cginc" // in UnityPBSLighting 
#include "UnityPBSLighting.cginc"

// Helper functions for lights. 
// Here light falloff for the point light is computed by transforming coordinates
// in the light's local space scaled by its attenuation. Anything further than 1
// from the light is considered out of range and thus attenuation = 0.
#include "AutoLight.cginc"


#ifdef HELIUM_BASE_COLOR
float4 _Color;
#endif

#ifdef HELIUM_EMISSION
float3 _EmissionColor;
sampler2D _Emission;
#endif

sampler2D _MainTex;
float4 _MainTex_ST;

float _UniformRoughness,_UniformMetallic;;
sampler2D _Metallic, _Roughness;

/*--Data Structs--*/

struct vInput{
    /*
    In order to use some Unity Macros, namely:
    - UNITY_LIGHT_ATTENUATION
    - SHADOW_ATTENUATION
    - TRANSFER_SHADOW
    - SHADOW_COORDS
    We needed to rename "csPos" to "pos" as it is the name
    of the field assuemd by the Unity macros.
    */
    float4 vertex: POSITION;
    float3 n : NORMAL;
    float2 uv : TEXCOORD0;
    float2 uvLight : TEXCOORD1;
    
    #ifdef HELIUM_NORMAL_MAPPING
    float4 tan : TANGENT;
    #endif
};

struct vOutput{

    /*
    In order to use some Unity Macros, namely:
    - UNITY_LIGHT_ATTENUATION
    - SHADOW_ATTENUATION
    - TRANSFER_SHADOW
    - SHADOW_COORDS
    We needed to rename "csPos" to "pos" as it is the name
    of the field assuemd by the Unity macros.
    */
    float4 pos : SV_Position; // Clip Space
    float3 n :   TEXCOORD0;
    #ifdef HELIUM_NORMAL_MAPPING
    float4 uvM : TEXCOORD1; // Main(xy) and Secondary(zw)
    #ifdef HELIUM_FRAGMENT_BINORMAL
    float4 tan : TEXCOORD3;
    #else
    float4 tan : TEXCOORD3;
    float3 bin : TEXCOORD4;
    #endif
    #else
    float2 uvM : TEXCOORD1; // Main
    #endif
    float4 wPos : TEXCOORD2; // World Space Position

    /*
    Same as 
    #ifdef SHADOWS_SCREEN
    float4 _ShadowCoord : TEXCOORD5; //<--- NAME IS IMPORTANT as it's assumed in the macros
    #endif
    */
    UNITY_SHADOW_COORDS(5) // 5 for the index of TEXCOORD 

    #ifdef VERTEXLIGHT_ON
    float3 lColor: TEXCOORD6; // Computed vertex light
    #elif defined(LIGHTMAP_ON)
    float2 uvLight : TEXCOORD6;
    #endif

};
/*-ALBEDO AND DETAIL ALBEDO-*/

#ifdef HELIUM_DETAIL_ALBEDO
sampler2D _SecondaryTex;
float4 _SecondaryTex_ST;
#endif

#ifdef HELIUM_DETAIL_MASK
sampler2D _DetailMask;
#endif

#ifdef HELIUM_DETAIL_MASK
    #define DETAIL_MASK(uv) tex2D(_DetailMask,uv.xy).a
    #define DETAIL_MASK_N(uv) tex2D(_DetailMask,uv.zw).a
#else
    #define DETAIL_MASK(uv) 1
    #define DETAIL_MASK_N(uv) 1
#endif

float3 ComputeAlbedoWithDetail(vOutput vo){
    float3 a = tex2D(_MainTex, vo.uvM.xy);

    #ifdef HELIUM_BASE_COLOR
    a *= _Color.xyz;
    #endif

    #ifdef HELIUM_NORMAL_MAPPING

    #ifdef HELIUM_DETAIL_ALBEDO
    float3 d = tex2D(_SecondaryTex, vo.uvM.zw) * unity_ColorSpaceDouble;
    #else
    float3 d = 1;
    #endif

    #ifdef HELIUM_DETAIL_MASK
    a = lerp(a, a * d, DETAIL_MASK(vo.uvM));
    #endif

    #endif

    return a;
}



/*-ROUGHNESS-*/
#if defined(HELIUM_R_FROM_METALLIC) && defined(HELIUM_2D_METALLIC)
    #define ROUGHNESS(uv) (1.0 - tex2D(_Metallic, uv.xy).a) * _UniformRoughness
#elif defined(HELIUM_R_FROM_ALBEDO)
    #define ROUGHNESS(uv) (1.0 - tex2D(_MainTex, uv.xy).a) * _UniformRoughness
#else
    #define ROUGHNESS(x) _UniformRoughness
#endif 

/*-EMISSION-*/
#ifdef HELIUM_COMPUTE_EMISSION
    #ifdef HELIUM_EMISSION_FROM_MAP
        #define EMISSION(uv) tex2D(_Emission, uv.xy) * _EmissionColor
    #else
        #define EMISSION(uv) _EmissionColor
    #endif
#else
    #define EMISSION(uv) 0
#endif

/*-METALLIC-*/
#ifdef HELIUM_2D_METALLIC
    #define METALLIC(uv) tex2D(_Metallic, uv.xy).r
#else
    #define METALLIC(x) _UniformMetallic
#endif 


/*-UTILITY FUNCTIONS-*/

// To prove TRANSFORM_TEX is just a define
#define HELIUM_TRANSFORM_TEX(x,y) (x.xy * y##_ST.xy + y##_ST.zw)

#define HELIUM_TRANSFORM_LIGHTMAP(uvVar, mapName) uvVar * mapName##ST.xy + mapName##ST.zw // suffix is different compared to normal TRASNFORM_TEX


#endif