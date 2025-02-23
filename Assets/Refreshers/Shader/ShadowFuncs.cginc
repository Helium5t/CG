
#ifndef HELIUM_SHADOWS
#define HELIUM_SHADOWS


// Technically not needed because CGPROGRAM always includes it https://docs.unity3d.com/6000.0/Documentation/Manual/shader-shaderlab-code-blocks.html
// but UnityClipSpaceShadowCasterPos does not get auto-included
#include "UnityCG.cginc"

struct svInput{
    float4 pos: POSITION;
    float3 n : NORMAL;
    // float2 uv : TEXCOORD0;
};

struct svOutput{
    float4 csPos : SV_Position; // Clip Space
    // float3 n :   TEXCOORD0;
    // float2 uvM : TEXCOORD1; // Main
    // float4 wPos : TEXCOORD5; // Worldspace Position
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
    return o;
}

half4 shadowFrag(): SV_Target{
    return 0;
}
#endif