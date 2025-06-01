#ifndef HELIUM_LIGHTMAPPING
#define HELIUM_LIGHTMAPPING

#define HELIUM_COMPUTE_EMISSION
#include "LightingCommon.cginc"
#include "UnityMetaPass.cginc"



vOutput vertLightMap(vInput v){
    vOutput o;
    // Rendering happens to the pixel on the lightmap, so we need to transform to the appropriate space
    v.vertex.xy = HELIUM_TRANSFORM_LIGHTMAP(v.uvLight, unity_Lightmap); // Get UV coordinates on the lightmap
    v.vertex.z = step(v.vertex.z, 0) * 0.0001; // Actually a dummy value, not actually used ultimately.

    o.pos = UnityObjectToClipPos(
        v.vertex
    );
    o.uvM.xy = TRANSFORM_TEX(v.uv, _MainTex);
    #ifdef HELIUM_DETAIL_ALBEDO
    o.uvM.zw = TRANSFORM_TEX(v.uv, _DetailTex);
    #endif
    return o;
}
 

float4 fragLightMap(vOutput vo) : SV_TARGET{
    UnityMetaInput surfaceMetaData; // Ad-hoc unity structure that will pass Albedo or emission to the lightmapper automatically
    surfaceMetaData.Emission = EMISSION(vo.uvM);
    float invertedReflectivity;
    float3 specularColor;
    surfaceMetaData.Albedo =  DiffuseAndSpecularFromMetallic(
        ComputeAlbedoWithDetail(vo),
        METALLIC(vo.uvM.xy),
        specularColor,
        invertedReflectivity
    );
    surfaceMetaData.SpecularColor = specularColor;

    // Account for the fact that very rough metals contribute to light a bit more
    float roughnessReflection = ROUGHNESS(vo.uvM);
    roughnessReflection = roughnessReflection * 0.5;
    surfaceMetaData.Albedo += specularColor * roughnessReflection;

    return UnityMetaFragment(surfaceMetaData);
}


#endif 