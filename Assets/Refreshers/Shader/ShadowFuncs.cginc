
#ifndef HELIUM_SHADOWS
#define HELIUM_SHADOWS


// Technically not needed because CGPROGRAM always includes it https://docs.unity3d.com/6000.0/Documentation/Manual/shader-shaderlab-code-blocks.html
// but UnityClipSpaceShadowCasterPos does not get auto-included
#include "UnityCG.cginc"

#if defined(HELIUM_TRANSPARENCY_BLENDED) || defined(HELIUM_TRANSPARENCY_TRANSLUCENT)
    #ifndef HELIUM_SHADOWS_FORCE_CUTOUT
        #define HELIUM_SHADOWS_DITHERED 1
    #else 
        #define HELIUM_TRANSPARENCY_CUTOUT
    #endif 
#endif 

#if (defined(HELIUM_SHADOWS_DITHERED) || defined(HELIUM_TRANSPARENCY_CUTOUT) ) && !defined(HELIUM_R_FROM_ALBEDO) // We can only use alpha when we want to have cutout transparency and alpha is not being used for roughness
    #define HELIUM_SHADOWS_SAMPLE_ALPHA 1
#endif


#ifdef HELIUM_SHADOWS_SAMPLE_ALPHA
    #define ALPHA(uv) _Color.a * tex2D(_Tex, uv.xy).a;
#else 
    #define ALPHA(uv) _Color.a;
#endif 

float4 _Color;
sampler2D _Tex;
float4 _Tex_ST;
float _AlphaThreshold;

sampler3D _DitherMaskLOD;

struct svInput{
    float4 pos: POSITION;
    float3 n : NORMAL;
    float2 uv : TEXCOORD0;
};

struct svOutput{
    float4 csPos : SV_Position; // Clip Space
    #ifdef HELIUM_SHADOWS_SAMPLE_ALPHA
        float2 uv : TEXCOORD0;
    #endif
    #ifdef SHADOWS_CUBE
        float3 lVec : TEXCOORD1; // Vector going from light to fragment 
    #endif
};

struct sfInput{
    #if HELIUM_SHADOWS_DITHERED
        UNITY_VPOS_TYPE ssPos : VPOS; // Screen-space position where "screen" is the shadow map. 
    #else 
        float4 pos : SV_POSITION;
    #endif
    #if HELIUM_SHADOWS_SAMPLE_ALPHA
        float2 uv : TEXCOORD0;
    #endif 
    #ifdef SHADOWS_CUBE
        float3 lVec : TEXCOORD1;
    #endif
};

svOutput shadowVert(svInput i){
    svOutput o;
    // UnityClipSpaceShadowCasterPos transforms direction also in clip space
    // and then moves the vertex by the normal bias amount (unity_LightShadowBias.z)
    o.csPos = UnityClipSpaceShadowCasterPos(i.pos, i.n);
    // in clip space the z is the distance so fundamentally the following also works
    /*
    o.csPos.z += saturate(unity_LightShadowBias.x / o.csPos.w);
    // correction for camera pos
    float clamped = max(o.csPos.z, o.csPos.w * UNITY_NEAR_CLIP_VALUE); 
    o.csPos.z = lerp(o.csPos.z, clamped, unity_LightShadowBias.y);
    */
    o.csPos = UnityApplyLinearShadowBias(o.csPos);
    #ifdef SHADOWS_CUBE
        o.lVec = mul(unity_ObjectToWorld, i.pos).xyz - _LightPositionRange.xyz;
    #endif 
    #ifdef HELIUM_SHADOWS_SAMPLE_ALPHA
        o.uv = TRANSFORM_TEX(i.uv, _Tex);
    #endif
    return o;
}

half4 shadowFrag(sfInput vo): SV_Target{
    float alpha = ALPHA(vo.uv);
    #ifdef HELIUM_TRANSPARENCY_CUTOUT
        clip(alpha - _AlphaThreshold);
    #endif
    #if HELIUM_SHADOWS_DITHERED
        float dither = tex3D(_DitherMaskLOD, float3(vo.ssPos.xy*0.25, alpha * 0.9375)).a;
        clip( dither - 0.01);
    #endif

    #ifdef SHADOWS_CUBE
        float depth = length(vo.lVec) + unity_LightShadowBias.x;
        depth *= _LightPositionRange.w; // .w = 1/(range of point light) so we divide by the range to get the final depth value
        return UnityEncodeCubeShadowDepth(depth);
    #else
        return 0.0;
    #endif
}
#endif