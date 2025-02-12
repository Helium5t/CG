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
#ifdef HELIUM_HEIGHT_MAPPING
sampler2D _Height;
// e.g. resolution is 1000x2000 => texel size is u=1/1000, v  = 1/2000
// the minimum amount of change for u and v that moves sampling to another pixel
float4 _Height_TexelSize;
#endif
#ifdef HELIUM_NORMAL_MAPPING
sampler2D _Normal, _SecondaryTex, _SecondaryNormal;
float _NormalStrength,_SecondaryNormalStrength;
float4 _SecondaryTex_ST;

#endif
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
    #ifdef HELIUM_NORMAL_MAPPING
    float4 uvM : TEXCOORD1; // Main(xy) and Secondary(zw)
    #else
    float2 uvM : TEXCOORD1; // Main
    #endif
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

    o.uvM = 0;
    o.uvM.xy = HELIUM_TRANSFORM_TEX(i.uv, _Tex);  // QOL command that summarizes texture tiling and offset
    #ifdef HELIUM_NORMAL_MAPPING
    o.uvM.zw = HELIUM_TRANSFORM_TEX(i.uv, _SecondaryTex);
    #endif
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

    // dimming is not declared because it's done inside the define.
    UNITY_LIGHT_ATTENUATION(dimming, 0, vo.wPos.xyz);
    l.color = _LightColor0  * dimming;
    // angle with surface normal
    l.ndotl =  DotClamped(vo.n, _WorldSpaceLightPos0.xyz);
    return l;
}

void InitFragNormal(inout vOutput vo){
    #if defined(HELIUM_HEIGHT_MAPPING)
    float2 du = float2(_Height_TexelSize.x * 0.5, 0);
    float u1 = tex2D(_Height, vo.uvM - du);
    float u2 = tex2D(_Height, vo.uvM + du);
    float2 dv = float2(0, _Height_TexelSize.y * 0.5);
    float v1 = tex2D(_Height, vo.uvM - dv);
    float v2 = tex2D(_Height, vo.uvM + dv);

    // Normal is the inverse of the tangent (rate of change)
    vo.n = float3(u2-u1, 1 , v2-v1); // Temporary TODO: delete, only applicable to plane
    vo.n = normalize(vo.n);
    // Tangent space transformation
    // float3 tv = float3(0, v2 - v1, 1);
    // float3 tu = float3(1, u2 - u1, 0);
    // float3x3 worldToTangent = transpose( float3x3(tu, tv, vo.n));
    // vo.wPos  = float4(mul(worldToTan, vo.n),1);
    #endif
    #ifdef HELIUM_NORMAL_MAPPING

    /*
    vo.n.xy = tex2D(_Normal, vo.uvM).wy *2 -1;
    vo.n.xy *= _NormalStrength;
    vo.n.z = sqrt(1 - saturate(dot(vo.n.xy, vo.n.xy)));
    */
    // Same as previous 3 lines
    vo.n = UnpackScaleNormal(tex2D(_Normal, vo.uvM.xy), -_NormalStrength); 
    float3 n2 = UnpackScaleNormal(tex2D(_SecondaryNormal, vo.uvM.zw), -_SecondaryNormalStrength);
    
    // Drawback of this approach is loss of detail for steeper slopers (the bigger the slope the greater the z thus the smaller weight in the addition)
    // vo.n = float3(  // Break the two normals in their respective x and y derivative components, add those and recompute normal
    //     vo.n.xy / vo.n.z + 
    //     n2.xy / n2.z,
    //     1
    // ) 
    // Use z instead as scaling factor
    // This will behave the opposite of previous approach and normals will be stronger for steeper slopes.
    // vo.n = float3(  // Break the two normals in their respective x and y derivative components, add those and recompute normal
    //     vo.n.xy +
    //     n2.xy,
    //     vo.n.z * n2.z
    // );
    // Same as previous line (whiteout blending)
    vo.n = BlendNormals(vo.n, n2);

    // Normal maps store the up direction in the z component 
    vo.n = vo.n.xzy;
    // Not needed anymore because of BlendNormals()
    // vo.n = normalize(vo.n.xzy);
    #endif
}

float4 frag(vOutput vo): SV_Target{
    float3 albedo =  tex2D(_Tex, vo.uvM.xy);

    #if defined(HELIUM_HEIGHT_MAPPING) || defined(HELIUM_NORMAL_MAPPING)
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
        
    #if !defined(HELIUM_HEIGHT_MAPPING)
        vo.n = normalize(vo.n);
    #endif
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
    // return finalDiffuse * albedo  /*Corrects linear to gamma transformation*/;
}



#endif