// Upgrade NOTE: upgraded instancing buffer 'InstanceProperties' to new syntax.

// Currently just replaces a variable name to allow Unity to handle shadows correctly.
#ifndef HELIUM_SHADOWS
#define HELIUM_SHADOWS

#include "HeliumMath.cginc"
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
    #define ALPHA(uv) UNITY_ACCESS_INSTANCED_PROP(_Color_arr, _Color).a * tex2D(_MainTex, uv.xy).a;
#else 
    #define ALPHA(uv) UNITY_ACCESS_INSTANCED_PROP(_Color_arr, _Color).a;
#endif 


#if defined(HELIUM_HEIGHT_MAP)
    #ifdef HELIUM_TESSELATE_ON_HEIGHT
        #define HELIUM_USE_TESSELATION_DISPLACEMENT
        #define _Displacement _ParallaxStrength

        #ifndef HELIUM_SHADOWS_SAMPLE_ALPHA
            #define HELIUM_SHADOWS_SAMPLE_ALPHA
        #endif 

    #endif
#endif

UNITY_INSTANCING_BUFFER_START(InstanceProperties)
UNITY_DEFINE_INSTANCED_PROP(float4,_Color)
#define _Color_arr InstanceProperties
UNITY_INSTANCING_BUFFER_END(InstanceProperties)
sampler2D _MainTex;
float4 _MainTex_ST;
float _Cutoff;
sampler2D _Height;
float _ParallaxStrength;

sampler3D _DitherMaskLOD;

struct svInput{
    UNITY_VERTEX_INPUT_INSTANCE_ID
    float4 vertex: POSITION;
    float3 n : NORMAL;
    float2 uv : TEXCOORD0;
};

struct svOutput{
    UNITY_VERTEX_INPUT_INSTANCE_ID
    float4 csPos : SV_Position; // Clip Space
    #ifdef HELIUM_SHADOWS_SAMPLE_ALPHA
        float2 uv : TEXCOORD0;
    #endif
    #ifdef SHADOWS_CUBE
        float3 lVec : TEXCOORD1; // Vector going from light to fragment 
    #endif
};

struct sfInput{
    UNITY_VERTEX_INPUT_INSTANCE_ID
    #if HELIUM_SHADOWS_DITHERED || defined(LOD_FADE_CROSSFADE)
        UNITY_VPOS_TYPE ssPos : VPOS; // Screen-space position where "screen" is the shadow map. 
    #else 
        float4 pos : SV_POSITION;
    #endif
    
    #ifdef HELIUM_SHADOWS_SAMPLE_ALPHA
        float2 uv : TEXCOORD0;
    #endif 
    #ifdef SHADOWS_CUBE
        float3 lVec : TEXCOORD1;
    #endif
};

svOutput shadowVert(svInput i){
    svOutput o;
    UNITY_TRANSFER_INSTANCE_ID(i,o);
    #ifdef HELIUM_SHADOWS_SAMPLE_ALPHA
        o.uv = TRANSFORM_TEX(i.uv, _MainTex);
    #endif

    #ifdef HELIUM_USE_TESSELATION_DISPLACEMENT
    float d = tex2Dlod(_Height, float4(o.uv.xy ,0,0)).g;
    d = (d - 0.5) * _Displacement;
    i.n = normalize(i.n);
    i.vertex.xyz += i.n * d;
    #endif
    #ifdef INSTANCING_ON
    unity_InstanceID = i.instanceID + unity_BaseInstanceID;
    #endif 
    // UnityClipSpaceShadowCasterPos transforms direction also in clip space
    // and then moves the vertex by the normal bias amount (unity_LightShadowBias.z)
    o.csPos = UnityClipSpaceShadowCasterPos(i.vertex, i.n);
    // in clip space the z is the distance so fundamentally the following also works
    /*
    o.csPos.z += saturate(unity_LightShadowBias.x / o.csPos.w);
    // correction for camera pos
    float clamped = max(o.csPos.z, o.csPos.w * UNITY_NEAR_CLIP_VALUE); 
    o.csPos.z = lerp(o.csPos.z, clamped, unity_LightShadowBias.y);
    */
    o.csPos = UnityApplyLinearShadowBias(o.csPos);
    #ifdef SHADOWS_CUBE
        o.lVec = mul(unity_ObjectToWorld, i.vertex).xyz - _LightPositionRange.xyz;
    #endif 
    return o;
}

half4 shadowFrag(sfInput vo): SV_Target{
    #ifdef INSTANCING_ON
    unity_InstanceID = vo.instanceID + unity_BaseInstanceID;
    #endif
    #if defined(LOD_FADE_CROSSFADE)
		UnityApplyDitherCrossFade(vo.ssPos);
	#endif
    float alpha = ALPHA(vo.uv);
    #if defined(HELIUM_TRANSPARENCY_CUTOUT) 
        clip(alpha - _Cutoff);
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