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
float4 _Tex_ST;

float _Roughness, _Metallic;
int _UseTextures;

struct vInput{
    float4 pos: POSITION;
    float3 n : NORMAL;
    float2 uv : TEXCOORD0;
};

struct vOutput{
    float4 csPos : SV_Position; // Clip Space
    float3 n :   TEXCOORD0;
    float2 uvM : TEXCOORD1; // Main
    float4 wPos : TEXCOORD2; // World Space Position

    #if defined(VERTEXLIGHT_ON)
    float3 lColor: TEXCOORD3; // Computed vertex light
    #endif
};

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

    o.uvM = HELIUM_TRANSFORM_TEX(i.uv, _Tex);  // QOL command that summarizes texture tiling and offset
    o.csPos = UnityObjectToClipPos(i.pos);
    o.wPos = mul(unity_ObjectToWorld, i.pos);

    o.n = UnityObjectToWorldNormal(i.n); 
    o.n = normalize(o.n);
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

    // dimming is not declared because it's done inside the define.
    UNITY_LIGHT_ATTENUATION(dimming, 0, vo.wPos.xyz);
    l.color = _LightColor0  * dimming;
    // angle with surface normal
    l.ndotl =  DotClamped(vo.n, _WorldSpaceLightPos0.xyz);
    return l;
}

float4 frag(vOutput vo): SV_Target{
    float3 albedo =  tex2D(_Tex, vo.uvM);

    UnityLight l = CreateLight(vo);

    UnityIndirect il = CreateIndirectLightAndDeriveFromVertex(vo);


    float invertedReflectivity;
    float3 specularColor;
    // specularColor and invertedReflectivity are out parameters
    albedo = DiffuseAndSpecularFromMetallic(
        albedo, _Metallic, specularColor, invertedReflectivity
    ); 
    float3 viewdir = normalize(_WorldSpaceCameraPos - vo.wPos);
    
    vo.n = normalize(vo.n);
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
    // return finalDiffuse * albedo  /*Corrects linear to gamma transformation*/;
}



#endif