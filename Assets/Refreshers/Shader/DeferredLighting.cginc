#ifndef HELIUM_DEFERRED_LIGHTS
#define HELIUM_DEFERRED_LIGHTS

// Includes UnityCG
#include "UnityPBSLighting.cginc"


#if !defined(SHADOWS_DEPTH) && !defined(SHADOWS_SCREEN) && !defined(SHADOWS_CUBE) && !defined(SHADOWS_SOFT) && !defined(SHADOWS_SHADOWMASK)
    #define HELIUM_SHADOWS_DISABLED
#endif

#if defined(DIRECTIONAL) || defined(DIRECTIONAL_COOKIE)
    #define HELIUM_DIRECTIONAL_LIGHT_MODEL
#endif

#if defined(POINT) || defined(POINT_COOKIE)
    #define HELIUM_POINT_LIGHT_MODEL
#endif

struct vInput{
    float4 pos : POSITION;
    float2 screenUV : TEXCOORD0; // UVs stretching over the screen. (Implemented engine side as a huge quad with each corner matching the screen's. Same as Deferred Fog.)
    float3 normal : NORMAL; // Given this will run over the stretched quad, Unity sets the quads normals to the rays we need. (from center of camera to the corner)
};

struct vOutput{
    float4 cPos : SV_POSITION; // Clip space
    float4 screenUVClipSpace : TEXCOORD0;
    float3 screenRay : TEXCOORD1;
};

/* ----- Unity built-in vars ----- */
float _LightAsQuad; // Actually a boolean telling us if we are rendering a full-screen quad or inferring light from light mesh vertices (pyramid case)
/* ------------------------------- */

vOutput vert(vInput v){
    vOutput o;
    o.cPos = UnityObjectToClipPos(v.pos);

    #ifdef HELIUM_DIRECTIONAL_LIGHT_MODEL
    // Actually we can do a little trick here to otpimize
    // We know we are working with a quad and it's object space coordinates match the screen space ones. 
    // In layman terms, each corner has the same coords of the corners of the screen (BL00,BR01, TL10, TR11)
    o.screenUVClipSpace = v.pos; // v is already in screen position
    o.screenRay = v.normal;
    #elif defined(SPOT)
    o.screenUVClipSpace = ComputeScreenPos(o.cPos);// from clip space to coordinates on the screen
    // Get the position of the pyramid vertices in camera space (0,0,0 centered in the camera, rotation aligned with the camera)
    // Then invert x and y (z is positive in Unity camera space so it's already inverted). 
    o.screenRay = lerp(
        UnityObjectToViewPos(v.pos) * float3(1,1,-1),
        v.normal,
        _LightAsQuad
    );
    #else 
    o.screenUVClipSpace = ComputeScreenPos(o.cPos);// from clip space to coordinates on the screen
    // Get the position of the pyramid vertices in camera space (0,0,0 centered in the camera, rotation aligned with the camera)
    // Then invert x and y (z is positive in Unity camera space so it's already inverted). 
    o.screenRay = lerp(
        UnityObjectToViewPos(v.pos) * float3(-1,-1,1),
        v.normal,
        _LightAsQuad
    );
    #endif

    return o;
}

/* ----- Unity built-in vars ----- */
sampler2D _LightBuffer; 
UNITY_DECLARE_DEPTH_TEXTURE(_CameraDepthTexture);
sampler2D _CameraGBufferTexture0;
sampler2D _CameraGBufferTexture1;
sampler2D _CameraGBufferTexture2;

#ifndef HELIUM_SHADOWS_DISABLED
    #ifdef SHADOWS_SCREEN
    sampler2D _ShadowMapTexture; 
    #endif
#endif
// Directional Light info
float4 _LightColor, _LightDir, _LightPos;
// Light Cookie Map and Spotlight fadeout map.
#ifdef POINT_COOKIE
samplerCUBE _LightTexture0;
#else
sampler2D _LightTexture0;
#endif
sampler2D _LightTextureB0;
float4x4 unity_WorldToLight; // World to light space
/* ------------------------------- */


#ifdef HELIUM_DEBUG_GBUFFERS
#define PI 3.14159265358979323846
#define SLANT 3.07768
#endif


#ifdef SHADOWS_SCREEN
float3 DirectionalShadowMap(float2 screenUV, float3 wPos, float depthViewSpace){
    float3 dim =  tex2D(_ShadowMapTexture, screenUV).r;
    return dim;
}
#endif

#ifdef SHADOWS_CUBE
float3 PointLightShadowMap(float3 lVec){
    float dim = UnitySampleShadowmap(-lVec);
    return dim;
}
#endif

#ifdef SHADOWS_DEPTH
float SpotlightShadowMap(float3 wPos){
    float dim = UnitySampleShadowmap(mul(unity_WorldToShadow[0], float4(wPos, 1)));
    return dim;
}
#endif

float LightCookie(float3 wPos){
    #if DIRECTIONAL_COOKIE
        float2 cookieUV = mul(unity_WorldToLight, float4(wPos, 1)).xy;
        return tex2Dbias(_LightTexture0, float4(cookieUV, 0, -8)).w; // This might not yield the desired result for some cookies and still show rims.
    #elif defined(SPOT) // Spot cookie is always enabled to get the fadeout
        float4 cookieUV = mul(unity_WorldToLight, float4(wPos, 1));
        cookieUV.xy /= cookieUV.w; // Account for the conic nature of spotlights
        float dim =  tex2Dbias(_LightTexture0, float4(cookieUV.xy, 0, -8)).w; // This might not yield the desired result for some cookies and still show rims.
        dim *= cookieUV.w < 0; // avoid rear facing cones
        return dim;
    #elif defined(POINT_COOKIE)
        float3 cookieUV = mul(unity_WorldToLight, float4(wPos, 1)).xyz;
        return texCUBEbias(
            _LightTexture0, float4(cookieUV, -8)
        ).w;
    #else
        return 1;
    #endif
}

#if defined(SPOT) || defined(HELIUM_POINT_LIGHT_MODEL)
float LightRangeFadeout(float3 lVec){
    return tex2D(
        _LightTextureB0,
        (dot(lVec,lVec) * _LightPos.w).rr
    ).UNITY_ATTEN_CHANNEL;
}
#endif

#ifndef HELIUM_SHADOWS_DISABLED
float MaxDistShadowFade(float3 wPos, float depthViewSpace, float dim, float maxDistFade){
    return saturate(dim + maxDistFade); // Add value to dimming to fade to white
}
#endif

float3 lightDir(float3 wPos){
    #ifdef HELIUM_DIRECTIONAL_LIGHT_MODEL
    return -_LightDir;
    #elif defined(SPOT) || defined(HELIUM_POINT_LIGHT_MODEL)
    return normalize(_LightPos.xyz - wPos);
    #else
    return float3(1.0,0.0,0.0);
    #endif
}

UnityLight ComputeDirectLighting(float2 screenUV, float3 wPos, float depthViewSpace){
    UnityLight l;

    l.dir = lightDir(wPos);
    float3 lVec = _LightPos.xyz - wPos;

    float shadowDimming = 1;

    float shadowFading = 0;
    #ifndef HELIUM_SHADOWS_DISABLED
    shadowFading = UnityComputeShadowFadeDistance(wPos, depthViewSpace);
    /*
    _LightShadowData : x - shadow strength | y - Appears to be unused | z - 1.0 / shadow far distance | w - shadow near distance
    */
    shadowFading = saturate(shadowFading * _LightShadowData.z + _LightShadowData.w);
    #endif
    
    #if defined(UNITY_FAST_COHERENT_DYNAMIC_BRANCHING) && defined(SHADOWS_SOFT)
        UNITY_BRANCH
        if (shadowFading > 0.99){
            shadowDimming = 1;
        }else{
    #endif
            #ifdef SHADOWS_SCREEN // SHADOWS_SCREEN is only used by directional light, POINT uses CUBE and SPOT uses DEPTH
            shadowDimming = DirectionalShadowMap(screenUV, wPos, depthViewSpace);
            #elif defined(SHADOWS_DEPTH) 
            shadowDimming *= SpotlightShadowMap(wPos);
            #elif defined(SHADOWS_CUBE)
            shadowDimming *= PointLightShadowMap(lVec);
            #endif
    #if defined(UNITY_FAST_COHERENT_DYNAMIC_BRANCHING) && defined(SHADOWS_SOFT)
        }
    #endif


    #ifndef HELIUM_SHADOWS_DISABLED 
    shadowDimming = MaxDistShadowFade(wPos, depthViewSpace, shadowDimming, shadowFading);
    #endif
    
    #if defined(SPOT) || defined(HELIUM_POINT_LIGHT_MODEL)
    shadowDimming *= LightRangeFadeout(lVec);
    #endif
    
    shadowDimming *= LightCookie(wPos);
    
    l.color = _LightColor.rgb * shadowDimming;
    // When computing the light we utilize the direction from the source to the light as this eases computations.
    // So we need to invert the _LightDir representation, as it represents the tavelling direction.
    return l;
}
float4 frag (vOutput i) : SV_Target
{
    float2 uvScreenSpace = i.screenUVClipSpace.xy / i.screenUVClipSpace.w;
    float d = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uvScreenSpace);
    d = Linear01Depth(d);

    /*the normal goes to the quad, which is aligned with the near plane, so we need to scale the vector*/
    float3 farPlaneScreenRay = (i.screenRay / i.screenRay.z/*scale down by near*/) * _ProjectionParams.z ;/*far plane value*/
    float3 fragViewSpace = farPlaneScreenRay * d;
    float3 fragWPos = mul(unity_CameraToWorld, float4(fragViewSpace, 1)).xyz;
    float3 worldSpaceViewD = normalize(_WorldSpaceCameraPos - fragWPos);
    
    
	float3 albedo = tex2D(_CameraGBufferTexture0, uvScreenSpace).rgb;
	float3 specularTint = tex2D(_CameraGBufferTexture1, uvScreenSpace).rgb;
	float3 smoothness = tex2D(_CameraGBufferTexture1, uvScreenSpace).a;
	float3 normal = tex2D(_CameraGBufferTexture2, uvScreenSpace).rgb * 2 - 1;
    float invertedReflectivity = 1 - SpecularStrength(specularTint);
    
    #ifdef HELIUM_DEBUG_GBUFFERS
    float lim0 = uvScreenSpace.x * SLANT - 0.25;
    float lim1 = uvScreenSpace.x * SLANT - 1.2;
    float lim2 = uvScreenSpace.x * SLANT - SLANT + 1;
    
    float3 c = albedo * step(lim0, uvScreenSpace.y) + normal * (1- step(lim0, uvScreenSpace.y));
    c = c * step(lim1,uvScreenSpace.y) + specularTint * (1 - step(lim1, uvScreenSpace.y));
    c = c * step(lim2,uvScreenSpace.y) + smoothness * (1 - step(lim2, uvScreenSpace.y));
    return float4(c,1.0);
    #endif
    
    UnityLight l = ComputeDirectLighting(uvScreenSpace, fragWPos, fragViewSpace);
    
    UnityIndirect il;
    il.diffuse = 0;
    il.specular = 0;
    
    float4 fCol = UNITY_BRDF_PBS(
        albedo,
        specularTint,
        invertedReflectivity,
        smoothness,
        normal,
        worldSpaceViewD,
        l,
        il
        );
        
    #ifndef UNITY_HDR_ON
    fCol = exp2(-fCol);
    #endif
    #ifdef DEBUG_SCREENRAY
    return normalize(float4(i.screenRay,1));
    #endif
    return fCol;

}

#endif