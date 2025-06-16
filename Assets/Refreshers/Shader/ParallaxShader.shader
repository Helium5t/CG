// Same as ShadowsPBShader but uses Unity naming convention
Shader "Refreshers/Parallax"
{   
    Properties{
        _Color("Color", Color) = (1,1,1,1)
        _MainTex ("Texture", 2D) = "white" {}
        [NoScaleOffset] _Normal ("Normal map", 2D) = "bump" {}
        _NormalStrength("Normal Strength", Float) = 1
        _SecondaryTex("Secondary Texture", 2D) = "gray"{}
        [NoScaleOffset] _SecondaryNormal("Secondary Normal map", 2D) = "bump" {}
        _SecondaryNormalStrength("Secondary Normal Strength", Float) = 1
        _UniformRoughness("Roughness", Range(0,1)) = 0.5
        _Roughness("Roughness",2D) = "white"{}
        [NoScaleOffset] _Metallic("Metallic", 2D) = "white" {}
        _UniformMetallic("Metallic", Range(0,1)) = 0.5
        _PackedMR("Metallic Roughness", 2D) = "grey" {}
        [NoScaleOffset] _Emission("Emission", 2D) = "black" {}
        _EmissionColor ("Emission Color", Color) = (0, 0, 0)
        [NoScaleOffset] _Occlusion ("Occlusion", 2D) = "white" {}
		_OcclusionStrength("Occlusion Strength", Range(0, 1)) = 1
        [NoScaleOffset] _Height ("Height Map", 2D) = "black" {}
        _ParallaxStrength("Parallax Strength", Range(0,1)) = 0
        [NoScaleOffset] _DetailMask ("Detail Mask", 2D) = "white" {}
        _Cutoff("Alpha Cutoff", Range(0,1)) = 0.5 
        [HideInInspector] _SourceBlend("_SourceBlend", Float) = 1
        [HideInInspector] _DestinationBlend("_DestinationBlend", Float) = 0
        [HideInInspector] _WriteToDepthBuffer("_WriteToDepthBuffer", Float) = 1
    }
    CGINCLUDE
    #define HELIUM_FRAGMENT_BINORMAL 1
    #define HELIUM_FOG_USE_WORLD_SPACE_DISTANCE 1

    ENDCG
    SubShader{
        Pass{
            Tags {
                "LightMode" = "ForwardBase"
			}
            Name "Standard Base FW"
            Blend [_SourceBlend] [_DestinationBlend]
            ZWrite [_WriteToDepthBuffer]

            CGPROGRAM
            #pragma target 3.0 // to enable BRDF
            #pragma vertex vert
            #pragma fragment frag
            
            
            // #pragma multi_compile _ VERTEXLIGHT_ON LIGHTMAP_ON
			// #pragma multi_compile _ SHADOWS_SCREEN 
            // #pragma multi_compile _ DIRLIGHTMAP_COMBINED
            #pragma multi_compile_fwdbase // Same as the lines above together
            // #pragma multi_compile DIRECTIONAL POINT SPOT DIRECTIONAL_COOKIE POINT_COOKIE
            #pragma multi_compile_fog
            #pragma multi_compile _ LOD_FADE_CROSSFADE
            #pragma multi_compile_instancing
            #pragma instancing_options lodfade


            #pragma shader_feature _ HELIUM_TRANSPARENCY_CUTOUT HELIUM_TRANSPARENCY_BLENDED HELIUM_TRANSPARENCY_TRANSLUCENT
            #pragma shader_feature HELIUM_2D_METALLIC
            #pragma shader_feature _ HELIUM_R_FROM_METALLIC HELIUM_R_FROM_ALBEDO
            #pragma shader_feature HELIUM_EMISSION_FROM_MAP
            #pragma shader_feature HELIUM_OCCLUSION_FROM_MAP
            #pragma shader_feature HELIUM_DETAIL_MASK

            #pragma shader_feature HELIUM_NORMAL_MAP
            #pragma shader_feature HELIUM_DETAIL_ALBEDO
            #pragma shader_feature HELIUM_DETAIL_NORMAL_MAP

            // For parallax
            #pragma shader_feature HELIUM_HEIGHT_MAP 
            
            #define HELIUM_NORMAL_MAPPING
            #define HELIUM_BASE_COLOR
            #define HELIUM_EMISSION
            #define HELIUM_AMBIENT_OCCLUSION
        
            #pragma multi_compile_fwdadd_fullshadows // equivalent of the following
            // #pragma multi_compile DIRECTIONAL POINT SPOT DIRECTIONAL_COOKIE POINT_COOKIE
            #pragma multi_compile_fog
            // #pragma multi_compile INSTANCING_ON and other keywords not used
            #pragma multi_compile_instancing
            #pragma instancing_options lodfade

			#include "LightingFuncsV3.cginc"

            ENDCG
        }
        Pass{
            Tags{
                "LightMode" = "ForwardAdd" // ForwardAdd makes it so this pass is "added" on top of the base one, used for the main light
            }
            Name "Standard Add FW"
            // In this case Blend 0 One One is the same as Blend One One 
            // since this shader is not using the other targets.
            // Blend 0 One One // Old Values

            Blend [_SourceBlend] One
            // In forward rendering, we only use the first (0 through 7) 
            // render target for the "main color" which would be the final look of a rendered image.
            // In deferred, all the targets are used for different things: 0 = Albedo, 1 = Normals etc.... 
            // So each element in the frame buffer would contain different info. 
            
            // Do not check and write to zbuffer as the depth check has been already run 
            // in the base pass.
            ZWrite Off 

            CGPROGRAM
            #pragma target 3.0 // to enable BRDF
            
            // Tells Unity's lighting helper 
            // functions that all macros will compute lighting based on the point light model
            // multi compile will create two compilations, 
            // one with #define POINT
            // and one with #define DIRECTIONAL
            // and one with #define SPOT
            // etc...
            //#pragma multi_compile DIRECTIONAL POINT SPOT DIRECTIONAL_COOKIE POINT_COOKIE
            // Equivalent of the one above
            #pragma multi_compile_fwdadd_fullshadows
            #pragma multi_compile_fog
            #pragma multi_compile _ LOD_FADE_CROSSFADE
            
            #define HELIUM_NORMAL_MAPPING
            #define HELIUM_ADD_PASS
            #define HELIUM_BASE_COLOR

            #pragma shader_feature _ HELIUM_TRANSPARENCY_CUTOUT HELIUM_TRANSPARENCY_BLENDED HELIUM_TRANSPARENCY_TRANSLUCENT
            #pragma shader_feature HELIUM_2D_METALLIC
            #pragma shader_feature _ HELIUM_R_FROM_METALLIC HELIUM_R_FROM_ALBEDO
            #pragma shader_feature HELIUM_DETAIL_MASK
            

            #pragma shader_feature HELIUM_NORMAL_MAP
            #pragma shader_feature HELIUM_DETAIL_ALBEDO
            #pragma shader_feature HELIUM_DETAIL_NORMAL_MAP
            
            #pragma vertex vert
            #pragma fragment frag
            
			#include "LightingFuncsV3.cginc"
            ENDCG
            

        }  
        Pass {
            Tags {
                "LightMode" = "Deferred"
            }
            Name "Standard Deferred"
            Blend [_SourceBlend] [_DestinationBlend]
            ZWrite [_WriteToDepthBuffer]

            CGPROGRAM
            #pragma target 3.0 // to enable BRDF
            #pragma exclude_renderers nomrt
            #pragma vertex vert
            #pragma fragment frag
            

			#pragma multi_compile _ SHADOWS_SCREEN 
            #pragma multi_compile _ LOD_FADE_CROSSFADE
            #pragma multi_compile_instancing
            #pragma instancing_options lodfade

            #pragma shader_feature _ HELIUM_TRANSPARENCY_CUTOUT // Not needed in deferred : HELIUM_TRANSPARENCY_BLENDED HELIUM_TRANSPARENCY_TRANSLUCENT
            #pragma shader_feature HELIUM_2D_METALLIC
            #pragma shader_feature _ HELIUM_R_FROM_METALLIC HELIUM_R_FROM_ALBEDO
            #pragma shader_feature HELIUM_EMISSION_FROM_MAP
            #pragma shader_feature HELIUM_OCCLUSION_FROM_MAP
            #pragma shader_feature HELIUM_DETAIL_MASK


            // #pragma multi_compile _ VERTEXLIGHT_ON LIGHTMAP_ON
            // #pragma shader_feature _ UNITY_HDR_ON
            // #pragma multi_compile _ DIRLIGHTMAP_COMBINED
            #pragma multi_compile_prepassfinal // Same as the lines above 

            #pragma shader_feature HELIUM_NORMAL_MAP
            #pragma shader_feature HELIUM_DETAIL_ALBEDO
            #pragma shader_feature HELIUM_DETAIL_NORMAL_MAP
            // For parallax
            #pragma shader_feature HELIUM_HEIGHT_MAP
            
            #define HELIUM_NORMAL_MAPPING
            #define HELIUM_BASE_COLOR
            #define HELIUM_EMISSION
            #define HELIUM_AMBIENT_OCCLUSION
        
            #pragma multi_compile_fwdadd_fullshadows // equivalent of the following
            // #pragma multi_compile DIRECTIONAL POINT SPOT DIRECTIONAL_COOKIE POINT_COOKIE

            #define HELIUM_DEFERRED_PASS
			#include "LightingFuncsV3.cginc"

            ENDCG
        }
        Pass{
            Tags{
                "LightMode" = "ShadowCaster"
            }
            Name "Standard Shadow"
            CGPROGRAM
            #pragma target 3.0
            #pragma vertex shadowVert
            #pragma fragment shadowFrag

            #pragma multi_compile_shadowcaster
            #pragma multi_compile _ LOD_FADE_CROSSFADE
            #pragma multi_compile_instancing
            #pragma instancing_options lodfade
            
            #pragma shader_feature _ HELIUM_TRANSPARENCY_CUTOUT HELIUM_TRANSPARENCY_BLENDED HELIUM_TRANSPARENCY_TRANSLUCENT
            #pragma shader_feature HELIUM_SHADOWS_FORCE_CUTOUT
            #pragma shader_feature HELIUM_R_FROM_ALBEDO
            
            #include "ShadowFuncsV2.cginc"

            ENDCG
        }
        Pass{ // Communicates with the Unity lightmapper to compute colored reflections
            Tags{
                "LightMode" = "Meta"
            }
            Cull Off

            Name "Standard Meta"
            CGPROGRAM
            #pragma vertex vertLightMap
			#pragma fragment fragLightMap
            #pragma multi_compile_fwdbase
            

            #pragma shader_feature HELIUM_2D_METALLIC
            #pragma shader_feature _ HELIUM_R_FROM_METALLIC HELIUM_R_FROM_ALBEDO
            #pragma shader_feature HELIUM_EMISSION_FROM_MAP
            #pragma shader_feature HELIUM_DETAIL_MASK

            #pragma shader_feature HELIUM_DETAIL_ALBEDO

            #define HELIUM_BASE_COLOR
            #define HELIUM_EMISSION
			
            #include "LightMappingFuncs.cginc"

			ENDCG
        }
    }
    Fallback "Diffuse"
    CustomEditor "HeliumShaderStandardUI"
}
