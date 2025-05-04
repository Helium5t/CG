/*
Same file as LightingFuncs.cginc but it used Unity naming convention 
in order to leverage its standard macros. Done in order to simplify code 
for future commits.

If notes or comments are missing here, they should be present for the same line
in "LightingFuncs.cginc"
*/
#if !defined(HELIUM_LIGHTING_INCLUDED)
#define HELIUM_LIGHTING_INCLUDED

#define HELIUM_MAX_LOD 6

// cginc because it is an include file
// #include "UnityCG.cginc" // Already included by UnityStandardBRDF
// #include "UnityStandardBRDF.cginc" // in UnityPBSLighting
// #include "UnityStandardUtils.cginc" // in UnityPBSLighting 
#include "UnityPBSLighting.cginc"

// Helper functions for lights. 
// Here light falloff for the point light is computed by transforming coordinates
// in the light's local space scaled by its attenuation. Anything further than 1
// from the light is considered out of range and thus attenuation = 0.
#include "AutoLight.cginc"

// To prove TRANSFORM_TEX is just a define
#define HELIUM_TRANSFORM_TEX(x,y) (x.xy * y##_ST.xy + y##_ST.zw)



sampler2D _Tex;

float _AlphaThreshold;

#ifdef HELIUM_NORMAL_MAPPING

    #ifdef HELIUM_DETAIL_ALBEDO
    sampler2D _SecondaryTex;
    float4 _SecondaryTex_ST;
    #endif

    #ifdef HELIUM_DETAIL_NORMAL_MAP
    sampler2D _SecondaryNormal;
    float _SecondaryNormalStrength;
    #endif 

    sampler2D _Normal;
    float _NormalStrength;
#endif

#ifdef HELIUM_BASE_COLOR
float4 _Color;
#endif

#ifdef HELIUM_EMISSION
float3 _EmissionColor;
sampler2D _Emission;
#endif

#ifdef HELIUM_AMBIENT_OCCLUSION
sampler2D _Occlusion;
float _OcclusionStrength;
#endif

#ifdef HELIUM_DETAIL_MASK
sampler2D _DetailMask;
#endif

float4 _Tex_ST;

float _UniformRoughness,_UniformMetallic;;
sampler2D _Metallic, _Roughness;

#ifdef HELIUM_2D_METALLIC
    #define METALLIC(uv) tex2D(_Metallic, uv.xy).r
#else
    #define METALLIC(x) _UniformMetallic
#endif 

#if defined(HELIUM_R_FROM_METALLIC) && defined(HELIUM_2D_METALLIC)
    #define ROUGHNESS(uv) (1.0 - tex2D(_Metallic, uv.xy).a) * _UniformRoughness
#elif defined(HELIUM_R_FROM_ALBEDO)
    #define ROUGHNESS(uv) (1.0 - tex2D(_Tex, uv.xy).a) * _UniformRoughness
#else
    #define ROUGHNESS(x) _UniformRoughness
#endif 

#if !defined(HELIUM_ADD_PASS)
    #ifdef HELIUM_EMISSION_FROM_MAP
        #define EMISSION(uv) tex2D(_Emission, uv.xy) * _EmissionColor
    #else
        #define EMISSION(uv) _EmissionColor
    #endif
#else
    #define EMISSION(uv) 0
#endif

#if !defined(HELIUM_ADD_PASS) && defined(HELIUM_AMBIENT_OCCLUSION)
    #ifdef HELIUM_OCCLUSION_FROM_MAP
        #define OCCLUSION(uv) lerp(1 ,tex2D(_Occlusion, uv.xy), _OcclusionStrength)
    #else
        #define OCCLUSION(uv) 1
    #endif
#else
    #define OCCLUSION(uv) 1
#endif

#ifdef HELIUM_DETAIL_MASK
    #define DETAIL_MASK(uv) tex2D(_DetailMask,uv.xy).a
    #define DETAIL_MASK_N(uv) tex2D(_DetailMask,uv.zw).a
#else
    #define DETAIL_MASK(uv) 1
    #define DETAIL_MASK_N(uv) 1
#endif

#ifndef HELIUM_R_FROM_ALBEDO
    #define ALPHA(uv) _Color.a * tex2D(_Tex, uv.xy).a
#else
    #define ALPHA(uv) _Color.a
#endif

int _UseTextures;

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
    float4 _ShadowCoord : TEXCOORD5; //<--- NAME IS IMPORTANT
    #endif
    */
    SHADOW_COORDS(5) // 5 for the index of TEXCOORD

    #if defined(VERTEXLIGHT_ON)
    float3 lColor: TEXCOORD6; // Computed vertex light
    #endif

};

float3 ComputeBinormal(float3 n, float3 t, float sign){
    return cross(n,t) * (sign * unity_WorldTransformParams.w/*Handles cases where object is mirrored in some dimension*/);
}

void ComputeVertexLight(inout vOutput v){
    #if defined(VERTEXLIGHT_ON)
    /*
        float3 lPos = float3(
            // unity_4LightPosA0 contains up to 4 vertex light information
            // where A = coordinate of light (0 is always 0)
            // X0.x => X coordinate of first light
            // Z0.y => Z coordinate of second light etc...
            unity_4LightPosX0.x,
            unity_4LightPosY0.x,
            unity_4LightPosZ0.x
        );
        float3 lv = lPos - v.wPos;
        float3 lDir = normalize(lv);
        float ndotl = DotClamped(v.n, lDir);
        // unity_4LightAtten0 approximates attenuation of pixel lights.
        // Essentialy aggregates overall attenuation
        float dimming = 1 / (1 + (dot(lv, lv) * unity_4LightAtten0.x));
      
        v.lColor = unity_LightColor[0] * ndotl * dimming;
    */
        // Does what's written above for each of the 4 vertex lights supported
        v.lColor = Shade4PointLights(
            unity_4LightPosX0,          // x coordinates of the 4 lights
            unity_4LightPosY0,          // y coordinates of the 4 lights
            unity_4LightPosZ0,          // z coordinates of the 4 lights
			unity_LightColor[0].rgb,    // Color light 1
            unity_LightColor[1].rgb,    // Color light 2
			unity_LightColor[2].rgb,    // Color light 3
            unity_LightColor[3].rgb,    // Color light 4   
			unity_4LightAtten0,         // Approximation of pixel lights
            v.wPos,
            v.n
        );

    #endif
}

vOutput vert(vInput i){
    vOutput o;

    o.uvM = 0;
    o.uvM.xy = HELIUM_TRANSFORM_TEX(i.uv, _Tex);  // QOL command that summarizes texture tiling and offset
    #ifdef HELIUM_NORMAL_MAPPING
        #ifdef HELIUM_DETAIL_ALBEDO
        o.uvM.zw = HELIUM_TRANSFORM_TEX(i.uv, _SecondaryTex);
        #endif
    o.tan = float4(UnityObjectToWorldDir(i.tan.xyz), i.tan.w);
    #ifndef HELIUM_FRAGMENT_BINORMAL
    o.bin = ComputeBinormal(i.n, i.tan.xyz, i.tan.w);
    #endif

    #endif
    o.pos = UnityObjectToClipPos(i.vertex);
    o.wPos = mul(unity_ObjectToWorld, i.vertex);

    o.n = UnityObjectToWorldNormal(i.n); 
    o.n = normalize(o.n);

    // Check LightingFuncs.cginc to see deeper explanation of how this works
    vInput v = i; // Unity assumes inpux from vertex shader is called v.
    TRANSFER_SHADOW(o);
    
    ComputeVertexLight(o);
    return o;
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


#define HELIUM_COMPUTE_REFLECTION(pn, a, b,c, destName) \
float3 rsv##pn = reflect(-a, b.n);\
rsv##pn = BoxProjectionIfActive(rsv##pn, b.wPos, unity_SpecCube##pn##_ProbePosition, unity_SpecCube##pn##_BoxMin, unity_SpecCube##pn##_BoxMax); \
/* Since they are all cubemaps of the same resolution etc..., 
    all specular probe cubemaps use the sampler from unity_SpecCube0 */ \
float4 specHDR##pn = UNITY_SAMPLE_TEXCUBE_SAMPLER_LOD(unity_SpecCube##pn , unity_SpecCube0, rsv##pn, c * ROUGHNESS(b.uvM.xy) * HELIUM_MAX_LOD); \
float3 destName = DecodeHDR(specHDR##pn, unity_SpecCube##pn##_HDR); 

UnityIndirect CreateIndirectLightAndDeriveFromVertex(vOutput vo, float3 viewDir){
    UnityIndirect il;
    il.diffuse =0;
    il.specular = 0;
    #if defined(VERTEXLIGHT_ON)
    il.diffuse = vo.lColor;
    #endif
    #if !defined(HELIUM_ADD_PASS)
    il.diffuse += max(0, ShadeSH9(float4(vo.n, 1)));
    
    float roughnessToMipMap = 1.7 - 0.7* ROUGHNESS(vo.uvM);
    // float3 reflectionSampleVec = reflect(-viewDir, vo.n);
    // reflectionSampleVec = BoxProjectionIfActive(reflectionSampleVec, vo.wPos, 
    //      unity_SpecCube0_ProbePosition, unity_SpecCube0_BoxMin, unity_SpecCube0_BoxMax);
    
    // // Approximates this https://s3.amazonaws.com/docs.knaldtech.com/knald/1.0.0/lys_power_drops.html
    // // fundamentally roughness should not be treated linearly because visibly it does not do it.
    // float4 specularHDR = UNITY_SAMPLE_TEXCUBE_LOD(unity_SpecCube0, reflectionSampleVec, roughnessToMipMap *  _Roughness * HELIUM_MAX_LOD);
    // float3 specular0 = DecodeHDR(specularHDR, unity_SpecCube0_HDR);//contains HDR decoding instructions
    // These two macros summarize the previous lines
    HELIUM_COMPUTE_REFLECTION(0, viewDir, vo,roughnessToMipMap, specular0)
    il.specular = specular0;
    #if UNITY_SPECCUBE_BLENDING
    UNITY_BRANCH
    if(unity_SpecCube0_BoxMin.w < 0.99){
        HELIUM_COMPUTE_REFLECTION(1, viewDir, vo,roughnessToMipMap, specular1)
        il.specular = lerp(specular1,il.specular, unity_SpecCube0_BoxMin.w);
    }
    #endif
    
    // Unity does the same thing via the Unity_GlossyEnvironmentData helper struct
    // Unity_GlossyEnvironmentData reflData;
    // reflData.roughness = _Roughness;
    // reflData.reflUVW = reflectionSampleVec;
    // il.specular = Unity_GlossyEnvironment(
    //     UNITY_PASS_TEXCUBE_SAMPLER(unity_SpecCubeX, unity_SpecCube0), unity_SpecCubeX_HDR, reflData
    // );

    float ao = OCCLUSION(vo.uvM);
    il.diffuse *= ao;
    il.specular *= ao;

    #endif
    return il;
}

UnityLight CreateLight(vOutput vo){
    UnityLight l;
    #if defined(POINT) || defined(SPOT) || defined(POINT_COOKIE)
        float3 lVector =  _WorldSpaceLightPos0.xyz - vo.wPos;
        l.dir = normalize( lVector);
    #else
        l.dir = _WorldSpaceLightPos0.xyz;
    #endif
    

    // Check LightingFuncs.cginc to better see how this works
    UNITY_LIGHT_ATTENUATION(dimming, vo, vo.wPos.xyz);
    l.color = _LightColor0  * dimming;
    // angle with surface normal
    l.ndotl =  DotClamped(vo.n, _WorldSpaceLightPos0.xyz);
    return l;
}

#ifdef HELIUM_NORMAL_MAPPING
float3 TanSpaceNormal(vOutput vo){
    float3 n1 = UnpackScaleNormal(tex2D(_Normal, vo.uvM.xy),-_NormalStrength); 

    #ifdef HELIUM_DETAIL_NORMAL_MAP
        float3 n2 = UnpackScaleNormal(tex2D(_SecondaryNormal, vo.uvM.zw), -_SecondaryNormalStrength);

        #ifdef HELIUM_DETAIL_MASK
        n2 = lerp(float3(0,0,1), n2, DETAIL_MASK_N(vo.uvM));
        #endif
        
        n1 = BlendNormals(n1, n2);
    #endif 

    return n1;
}
#endif

void InitFragNormal(inout vOutput vo){
    #ifdef HELIUM_NORMAL_MAPPING
    float3 tanSpaceNormal = TanSpaceNormal(vo);
    // Normal maps store the up direction in the z component 
    tanSpaceNormal = tanSpaceNormal.xzy;
    #ifdef HELIUM_FRAGMENT_BINORMAL
    float3 bn = ComputeBinormal(vo.n, vo.tan.xyz, vo.tan.w);
    #else
    float3 bn = vo.bin;
    #endif
    vo.n = normalize(
        tanSpaceNormal.x * vo.tan + 
        tanSpaceNormal.y * vo.n +
        tanSpaceNormal.z * bn
    );
    #endif
}

float3 ComputeAlbedoWithDetail(vOutput vo){
    float3 a = tex2D(_Tex, vo.uvM.xy);

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


float4 frag(vOutput vo): SV_Target{
    float alpha = ALPHA(vo.uvM);
    #ifdef HELIUM_TRANSPARENCY_CUTOUT
    clip(alpha-_AlphaThreshold);
    #endif

    float3 albedo =  ComputeAlbedoWithDetail(vo);

    #ifdef HELIUM_NORMAL_MAPPING
    InitFragNormal(vo);
    #endif

    
    float invertedReflectivity;
    float3 specularColor;
    float m = METALLIC(vo.uvM.xy);
    // specularColor and invertedReflectivity are out parameters
    albedo = DiffuseAndSpecularFromMetallic(
        albedo, m, specularColor, invertedReflectivity
        ); 
        float3 viewdir = normalize(_WorldSpaceCameraPos - vo.wPos);
    #ifdef HELIUM_TRANSPARENCY_TRANSLUCENT
    albedo *= alpha;
    alpha = 1 - invertedReflectivity + alpha * invertedReflectivity;
    #endif
    vo.n = normalize(vo.n);
    float3 approximatedCol = ShadeSH9(float4(vo.n, 1));
    
    UnityLight l = CreateLight(vo);
    UnityIndirect il = CreateIndirectLightAndDeriveFromVertex(vo,viewdir);
    float4 finalCol = UNITY_BRDF_PBS(
    albedo,
    specularColor,
    invertedReflectivity,
    1.0 - ROUGHNESS(vo.uvM),
    vo.n,
    viewdir,
    l,
    il
    );
    
    finalCol.rgb += EMISSION(vo.uvM);
    #if defined(HELIUM_TRANSPARENCY_BLENDED) || defined(HELIUM_TRANSPARENCY_TRANSLUCENT)
    finalCol.a = alpha;
    #endif

    return finalCol;
}



#endif