
#ifndef HELIUM_SHADOWS
#define HELIUM_SHADOWS


// Technically not needed because CGPROGRAM always includes it https://docs.unity3d.com/6000.0/Documentation/Manual/shader-shaderlab-code-blocks.html
// but UnityClipSpaceShadowCasterPos does not get auto-included
#include "UnityCG.cginc"

struct svInput{
    float4 pos: POSITION;
    float3 n : NORMAL;
};

struct svOutput{
    float4 csPos : SV_Position; // Clip Space
    #ifdef SHADOWS_CUBE
        float3 lVec : TEXCOORD0; // Vector going from light to fragment 
    #endif
};

svOutput shadowVert(svInput i){
    svOutput o;
    #ifdef SHADOWS_CUBE
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
        o.lVec = mul(unity_ObjectToWorld, i.pos).xyz - _LightPositionRange.xyz;
    #else 
        o.csPos = UnityClipSpaceShadowCasterPos(i.pos, i.n);
        o.csPos = UnityApplyLinearShadowBias(o.csPos);
    #endif 
    return o;
}

half4 shadowFrag(svOutput vo): SV_Target{
    #ifdef SHADOWS_CUBE
        float depth = length(vo.lVec) + unity_LightShadowBias.x;
        depth *= _LightPositionRange.w; // .w = 1/(range of point light) so we divide by the range to get the final depth value
        return UnityEncodeCubeShadowDepth(depth);
    #else
        return 0.0;
    #endif
}
#endif