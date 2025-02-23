/*
Same file as LightingFuncs.cginc but it used Unity naming convention 
in order to leverage its standard macros. Done in order to simplify code 
for future commits.

If notes or comments are missing here, they should be present for the same line
in "LightingFuncs.cginc"
*/
#if !defined(HELIUM_LIGHTING_INCLUDED)
#define HELIUM_LIGHTING_INCLUDED


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
#ifdef HELIUM_NORMAL_MAPPING
sampler2D _Normal, _SecondaryTex, _SecondaryNormal;
float _NormalStrength,_SecondaryNormalStrength;
float4 _SecondaryTex_ST;

#endif
#ifdef HELIUM_BASE_COLOR
float4 _Color;
#endif

float4 _Tex_ST;

float _Roughness, _Metallic;
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
    o.uvM.zw = HELIUM_TRANSFORM_TEX(i.uv, _SecondaryTex);
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

UnityIndirect CreateIndirectLightAndDeriveFromVertex(vOutput vo){
    UnityIndirect il;
    il.diffuse =0;
    il.specular = 0;
    #if defined(VERTEXLIGHT_ON)
        il.diffuse = vo.lColor;
    #endif
    #if !defined(HELIUM_ADD_PASS)
        il.diffuse += max(0, ShadeSH9(float4(vo.n, 1)));
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

void InitFragNormal(inout vOutput vo){
    #ifdef HELIUM_NORMAL_MAPPING
    float3 n1 = UnpackScaleNormal(tex2D(_Normal, vo.uvM.xy), -_NormalStrength); 
    float3 n2 = UnpackScaleNormal(tex2D(_SecondaryNormal, vo.uvM.zw), -_SecondaryNormalStrength);
    float3 tanSpaceNormal = BlendNormals(n1, n2);

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

float4 frag(vOutput vo): SV_Target{
    float3 albedo =  tex2D(_Tex, vo.uvM.xy);


    #ifdef HELIUM_BASE_COLOR
    albedo *= _Color.xyz;
    #endif

    #ifdef HELIUM_NORMAL_MAPPING
    InitFragNormal(vo);
    #endif

    #ifdef HELIUM_NORMAL_MAPPING

    albedo *= tex2D(_SecondaryTex, vo.uvM.zw) * unity_ColorSpaceDouble;
    #endif
    
    
    float invertedReflectivity;
    float3 specularColor;
    // specularColor and invertedReflectivity are out parameters
    albedo = DiffuseAndSpecularFromMetallic(
        albedo, _Metallic, specularColor, invertedReflectivity
        ); 
        float3 viewdir = normalize(_WorldSpaceCameraPos - vo.wPos);
        
    vo.n = normalize(vo.n);
    float3 approximatedCol = ShadeSH9(float4(vo.n, 1));
    
    UnityLight l = CreateLight(vo);
    UnityIndirect il = CreateIndirectLightAndDeriveFromVertex(vo);
    return UNITY_BRDF_PBS(
    albedo,
    specularColor,
    invertedReflectivity,
    1.0-_Roughness,
    vo.n,
    viewdir,
    l,
    il
    );
}



#endif