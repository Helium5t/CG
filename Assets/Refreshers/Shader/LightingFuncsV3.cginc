/*
An iteration of LightingFuncsV2B. 
Uses unity's standard convetion to allow leveraging the macros. 

Compared to V2B Adds:
 - Deferred rendering support
 - Fog


If notes or comments are missing here, they should be present for the same line
in "Archive/LightingFuncs.cginc"
*/

#ifndef HELIUM_LIGHTING_INCLUDED
#define HELIUM_LIGHTING_INCLUDED

#include "LightingCommon.cginc"
#include "HeliumMaterialMacros.cginc"

#include "HeliumMath.cginc"

// Alpha threshold to clip the pixel. Called like this because Unity wouldn't be able to handle shadows otherwise.
float _Cutoff;

#ifdef HELIUM_HEIGHT_MAP 
sampler2D _Height;
float _ParallaxStrength;
#endif

#ifdef HELIUM_DETAIL_NORMAL_MAP
sampler2D _SecondaryNormal;
float _SecondaryNormalStrength;
#endif 

sampler2D _Normal;
float _NormalStrength;


#ifdef HELIUM_AMBIENT_OCCLUSION
sampler2D _Occlusion;
float _OcclusionStrength;
#endif

int _UseTextures;

struct fOutput{
    #ifdef HELIUM_DEFERRED_PASS
    float4 g0 : SV_Target0;
    float4 g1 : SV_Target1;
    float4 g2 : SV_Target2;
    float4 g3 : SV_Target3;

    #ifdef HELIUM_SHADOWMASK_ENABLED // Some platforms don't support 5+ gbuffers
    float4 g4 : SV_Target4; // gBuffer reserved for the shadowmask when active
    #endif

    #else
    float4 colorOut  : SV_Target;
    #endif
};

#if defined(VERTEXLIGHT_ON)
void ComputeVertexLight(inout vOutput v){
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

    }
#endif

vOutput vert(vInput i){
    vOutput o;
    
    o.uvM = 0;
    o.uvM.xy = HELIUM_TRANSFORM_TEX(i.uv, _MainTex);  // QOL command that summarizes texture tiling and offset
    #ifdef HELIUM_DETAIL_ALBEDO
    o.uvM.zw = HELIUM_TRANSFORM_TEX(i.uv, _SecondaryTex);
    #endif
    o.tan = float4(UnityObjectToWorldDir(i.tan.xyz), i.tan.w);
    #ifndef HELIUM_FRAGMENT_BINORMAL
    o.bin = ComputeBinormal(i.n, i.tan.xyz, i.tan.w);
    
    #endif
    
    UNITY_TRANSFER_INSTANCE_ID(i, o); // In case of instancing, set the transfer ID for the fragment shader struct
    #ifdef INSTANCING_ON
    // Same as UNITY_SETUP_INSTANCE_ID
    unity_InstanceID = i.instanceID + unity_BaseInstanceID; // fetch the correct instance for mvp matrix selection
    #endif
    o.pos = UnityObjectToClipPos(i.vertex);
    o.wPos = mul(unity_ObjectToWorld, i.vertex);

    o.n = UnityObjectToWorldNormal(i.n); 
    o.n = normalize(o.n);

    // Check LightingFuncs.cginc to see deeper explanation of how this works
    vInput v = i; // Unity assumes input from vertex shader is called v.
    UNITY_TRANSFER_SHADOW(o, i.uvLight);
    
    #ifdef VERTEXLIGHT_ON
    ComputeVertexLight(o);
    #elif defined(LIGHTMAP_ON) || defined(HELIUM_MULTIPLE_DIRECTIONAL_SHADOWMASKS)
    o.uvLight = HELIUM_TRANSFORM_LIGHTMAP(i.uvLight, unity_Lightmap); 
    #endif

    #ifdef DYNAMICLIGHTMAP_ON
    o.uvDynLight = HELIUM_TRANSFORM_LIGHTMAP(i.uvDynLight, unity_DynamicLightmap);
    #endif

    #ifdef HELIUM_HEIGHT_MAP
    /* if batching breaks things use this
    i.tan.xyz = normalize(i.tan.xyz);
    i.n = normalize(i.n);
    */
    float3x3 objToTan = float3x3(
        i.tan.xyz, 
        cross(i.n, i.tan.xyz) * i.tan.w, 
        i.n);
    o.viewDirTanSpace = mul(objToTan, ObjSpaceViewDir(i.vertex));
    #endif

    return o;
}

#ifdef HELIUM_APPROX_SUBTRACTIVE_LIGHTING
void ApplySubtractiveLighting(fInput vo, inout UnityIndirect il){
    UNITY_LIGHT_ATTENUATION(dimming, vo, vo.wPos.xyz);
    dimming = ComputeShadowFading(vo, dimming);

    float nl = saturate(
        dot(
            vo.n, _WorldSpaceLightPos0.xyz
        ));
    float3 dimmedLightApprox = nl * (1-dimming) * _LightColor0.rgb;
    dimmedLightApprox = max(dimmedLightApprox, unity_ShadowColor.rgb);
    dimmedLightApprox = lerp(
        dimmedLightApprox, il.diffuse, _LightShadowData.x // _LightShadowData.x is shadow strength
    );
    float3 l = il.diffuse - dimmedLightApprox;
    il.diffuse = min(l, il.diffuse);
}
#endif

UnityIndirect CreateIndirectLightAndDeriveFromVertex(fInput vo, float3 viewDir){
    UnityIndirect il;
    il.diffuse =0;
    il.specular = 0;

    #if defined(VERTEXLIGHT_ON)
    il.diffuse = vo.lColor;
    #endif

    #if !defined(HELIUM_ADD_PASS) || defined(HELIUM_DEFERRED_PASS)

        #ifdef LIGHTMAP_ON 
            // overrides vertexlight value, although we don't expect to have it since Unity either defined VERTEXLIGHT_ON or LIGHTMAP_ON
            il.diffuse = DecodeLightmap(UNITY_SAMPLE_TEX2D(
                unity_Lightmap, vo.uvLight
            ));
            #ifdef DIRLIGHTMAP_COMBINED // We have direction info from lightmap
                float4 lightmapDir = UNITY_SAMPLE_TEX2D_SAMPLER(
                    unity_LightmapInd, unity_Lightmap, vo.uvLight
                );
                il.diffuse = DecodeDirectionalLightmap(
                    il.diffuse, lightmapDir, vo.n
                );
            #endif
            
            #ifdef HELIUM_APPROX_SUBTRACTIVE_LIGHTING
                ApplySubtractiveLighting(vo, il);
            #endif
        #endif
            
        #ifdef DYNAMICLIGHTMAP_ON
            float3 dynLightDiffuse = DecodeRealtimeLightmap(
                UNITY_SAMPLE_TEX2D(unity_DynamicLightmap, vo.uvDynLight)
            );

            #ifdef DIRLIGHTMAP_COMBINED // We have direction info from lightmap
                float4 dynLightmapDir = UNITY_SAMPLE_TEX2D_SAMPLER(
                    unity_DynamicDirectionality, unity_DynamicLightmap, vo.uvDynLight
                );
                il.diffuse += DecodeDirectionalLightmap(
                    dynLightDiffuse, dynLightmapDir, vo.n
                );
            #else
                il.diffuse += dynLightDiffuse;
            #endif
        #endif
            
        #ifdef HELIUM_NO_LIGHTMAPS // Previous blocks did not run
			float3 harmonics = ShadeSH9(float4(vo.n, 1));
            #ifdef UNITY_LIGHT_PROBE_PROXY_VOLUME
                // big function that does harmonics but with only 2 lights and sampling over multple points in world space => EXPENSIVE :C
                float3 lppv = SHEvalLinearL0L1_SampleProbeVolume(
                    float4(vo.n, 1), vo.wPos
                );
                il.diffuse += max(0, lppv * step(0.1, unity_ProbeVolumeParams.x) + harmonics * (1-step(0.1, unity_ProbeVolumeParams.x)));
				#ifdef UNITY_COLORSPACE_GAMMA
				il.diffuse = LinearToGammaSpace(il.diffuse);
				#endif
            #else
                il.diffuse += max(0,harmonics);
            #endif
        #endif

    
        float roughnessToMipMap = 1.7 - 0.7* ROUGHNESS(vo.uvM);
        // float3 reflectionSampleVec = reflect(-viewDir, vo.n);
        // reflectionSampleVec = BoxProjectionIfActive(reflectionSampleVec, vo.wPos, 
        //      unity_SpecCube0_ProbePosition, unity_SpecCube0_BoxMin, unity_SpecCube0_BoxMax);
    
        // // Approximates this https://s3.amazonaws.com/docs.knaldtech.com/knald/1.0.0/lys_power_drops.html
        // // fundamentally roughness should not be treated linearly because visually it does not behave so.
        // float4 specularHDR = UNITY_SAMPLE_TEXCUBE_LOD(unity_SpecCube0, reflectionSampleVec, roughnessToMipMap *  _Roughness * HELIUM_MAX_LOD);
        // float3 specular0 = DecodeHDR(specularHDR, unity_SpecCube0_HDR);//contains HDR decoding instructions
        // These two macros summarize the previous lines
        #if defined(DEFERRED_PASS) && UNITY_ENABLE_REFLECTION_BUFFERS
            
        il.specular = 0;

        #else

        HELIUM_COMPUTE_REFLECTION(0, viewDir, vo,roughnessToMipMap, specular0)
        il.specular = specular0;
        #if UNITY_SPECCUBE_BLENDING
        UNITY_BRANCH
        if(unity_SpecCube0_BoxMin.w < 0.99){
            HELIUM_COMPUTE_REFLECTION(1, viewDir, vo,roughnessToMipMap, specular1)
            il.specular = lerp(specular1,il.specular, unity_SpecCube0_BoxMin.w);
        }
        #endif

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

#if defined(HANDLE_SHADOWS_BLENDING_IN_GI) || defined(HELIUM_MULTIPLE_DIRECTIONAL_SHADOWMASKS) // Unity variable telling us whether GI is handling shadows, which usually breaks the fading.
float ComputeShadowFading(vOutput vo, float dimming){
    float viewSpaceZ = dot(_WorldSpaceCameraPos - vo.wPos, UNITY_MATRIX_V[2].xyz);
    float bakedShadow = UnitySampleBakedOcclusion(vo.uvLight, vo.wPos);
    // Depending on Unity's Light fit type (Close or Stable fit) we need to use a different value to know how much to fade the shadow
    float fadeOffset = UnityComputeShadowFadeDistance(vo.wPos, viewSpaceZ); 
    fadeOffset = UnityComputeShadowFade(fadeOffset);
    dimming = UnityMixRealtimeAndBakedShadows(
        dimming, bakedShadow, fadeOffset
    );
    return dimming;
}
#endif

UnityLight CreateLight(fInput vo){
    UnityLight l;
    #if defined(HELIUM_DEFERRED_PASS) || defined(HELIUM_APPROX_SUBTRACTIVE_LIGHTING)
        l.dir = float3(0,1,0);
        l.color = 0.0;
    #else
        #if defined(POINT) || defined(SPOT) || defined(POINT_COOKIE)
            float3 lVector =  _WorldSpaceLightPos0.xyz - vo.wPos;
            l.dir = normalize( lVector);
        #else
            l.dir = _WorldSpaceLightPos0.xyz;
        #endif
    
        // Check LightingFuncs.cginc to better see how this works
        #ifndef HELIUM_MULTIPLE_DIRECTIONAL_SHADOWMASKS
        UNITY_LIGHT_ATTENUATION(dimming, vo, vo.wPos.xyz);
        #else
        dimming = SHADOW_ATTENUATION(vo);
        #endif

        #if defined(HANDLE_SHADOWS_BLENDING_IN_GI) || defined(HELIUM_MULTIPLE_DIRECTIONAL_SHADOWMASKS)
        dimming = ComputeShadowFading(vo, dimming);
        #endif

        l.color = _LightColor0  * dimming;
        // angle with surface normal
    #endif 
    return l;
}

float3 TanSpaceNormal(fInput vo){
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

void InitFragNormal(inout fInput vo){
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
}

#ifdef HELIUM_HEIGHT_MAP
float GetParallaxHeight (float2 uv) {
	return tex2D(_Height, uv).g;
}
float2 ParallaxOffset (float2 uv, float2 viewDir) {
	float height = GetParallaxHeight(uv);
	height -= 0.5;
	height *= _ParallaxStrength;
	return viewDir * height;
}
float2 ParallaxRaymarching (float2 uv, float2 viewDir) {
	float2 uvOffset = 0;
	float stepSize = 0.01;
	float2 uvDelta = viewDir * (stepSize * _ParallaxStrength);

    float stepHeight = 1;
	float surfaceHeight = GetParallaxHeight(uv);
    for (int i = 1; i < 100 && stepHeight > surfaceHeight; i++) {
		uvOffset -= uvDelta;
		stepHeight -= stepSize;
		surfaceHeight = GetParallaxHeight(uv + uvOffset);
	}
	return uvOffset;
}

#define PARALLAX_FUNCTION ParallaxRaymarching

#endif

void ApplyParallax (inout fInput i) {
	#if defined(HELIUM_HEIGHT_MAP)
		i.viewDirTanSpace = normalize(i.viewDirTanSpace);
		#if !defined(PARALLAX_OFFSET_LIMITING)
			#if !defined(PARALLAX_BIAS)
				#define PARALLAX_BIAS 0.42
			#endif
			i.viewDirTanSpace.xy /= (i.viewDirTanSpace.z + PARALLAX_BIAS);
		#endif
		
		#if !defined(PARALLAX_FUNCTION)
			#define PARALLAX_FUNCTION ParallaxOffset
		#endif
		float2 uvOffset = PARALLAX_FUNCTION(i.uvM.xy, i.viewDirTanSpace.xy);
		i.uvM.xy += uvOffset;
		i.uvM.zw += uvOffset * (_SecondaryTex_ST.xy / _MainTex_ST.xy);
	#endif
}

fOutput frag(fInput vo){
    fOutput fout;
    // vo.n = normalize(cross(ddx(vo.wPos), ddy(vo.wPos))); <---- flat shading. ddx and ddy get the field difference between this fragment and the one to the right (ddx) and to the top (ddy).
    #ifdef INSTANCING_ON
    unity_InstanceID = vo.instanceID + unity_BaseInstanceID; // fetch the correct instance for mvp matrix selection
    #endif
	#ifdef LOD_FADE_CROSSFADE
		UnityApplyDitherCrossFade(vo.lodVPos);
	#endif

    #ifdef HELIUM_HEIGHT_MAP
        // DisplaceUVParallax(vo.uvM.xy,vo.viewDirTanSpace, (tex2D(_Height, vo.uvM.xy).r - 0.5)*  _ParallaxStrength,_ParallaxStrength);
        DisplaceUVParallax(vo.uvM,vo.viewDirTanSpace, _Height, _ParallaxStrength,  (_SecondaryTex_ST.xy / _MainTex_ST.xy) );
    #endif
    // #ifdef HELIUM_HEIGHT_MAP
    // ApplyParallax(vo);
    // #endif
    
    float alpha = ALPHA(vo.uvM);
    #ifdef HELIUM_TRANSPARENCY_CUTOUT
    clip(alpha-_Cutoff);
    #endif

    float3 albedo =  ComputeAlbedoWithDetail(vo);

    InitFragNormal(vo);

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

    #if defined(HELIUM_PAINT_WIREFRAME)
    float3 minB = float3(vo.baryCoord, 1 - vo.baryCoord.x - vo.baryCoord.y);
    float3 dd = fwidth(minB);
	minB = smoothstep(dd, 2 * dd, minB * vo.pos.w/*Accounts for screen size of the fragment*/);
	float minBary = min(minB.x, min(minB.y, minB.z));
	finalCol = finalCol * minBary + float4(_WFColor,1.0) * (1 - minBary);
    #endif


    #ifdef HELIUM_DEFERRED_PASS
        float ao = OCCLUSION(vo.uvM);
        fout.g0 = float4(
            albedo.rgb,
            ao) ;
        fout.g1 = float4(
            specularColor,
            1.0 - ROUGHNESS(vo.uvM)
        );
        fout.g2 = float4(
            vo.n * 0.5 + 0.5, 1.0);
        #ifndef UNITY_HDR_ON
            finalCol.rgb = exp2(-finalCol.rgb);
        #endif
        fout.g3 = finalCol;
        
        #ifdef HELIUM_SHADOWMASK_ENABLED
        float2 uv = 0;
            #ifdef LIGHTMAP_ON
            uv = vo.uvLight;
            #endif
        fout.g4 = UnityGetRawBakedOcclusions(
           uv, vo.wPos.xyz
        );
        #endif
        return fout;
    #else
        // finalCol =   (vo.pos.z/vo.pos.w);
        fout.colorOut = finalCol;
        HELIUM_COMPUTE_FOG(fout.colorOut, vo);
        return fout; 
    #endif
}



#endif