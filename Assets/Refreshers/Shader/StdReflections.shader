// Same as ShadowsPBShader but uses Unity naming convention
Shader "Refreshers/StdReflections"
{   
    Properties{
        _Color("Color", Color) = (1,1,1,1)
        _Tex ("Texture", 2D) = "white" {}
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
    }
    CGINCLUDE
    #define HELIUM_FRAGMENT_BINORMAL
    ENDCG
    SubShader{

        Pass{
            Tags {
				"LightMode" = "ForwardBase"
			}
            CGPROGRAM
            #pragma target 3.0 // to enable BRDF
            #pragma vertex vert
            #pragma fragment frag
            
            // Compile a version that computes light per vertex, much cheaper than per fragment.
            // Only point is supported
            #pragma multi_compile _ VERTEXLIGHT_ON

			#pragma multi_compile _ SHADOWS_SCREEN 

            #pragma shader_feature HELIUM_2D_METALLIC
            #pragma shader_feature HELIUM_2D_ROUGHNESS
            #pragma shader_feature HELIUM_PACKED_MR

            #define HELIUM_NORMAL_MAPPING
            #define HELIUM_BASE_COLOR
        
            #pragma multi_compile_fwdadd_fullshadows // equivalent of the following
            // #pragma multi_compile DIRECTIONAL POINT SPOT DIRECTIONAL_COOKIE POINT_COOKIE

			#include "LightingFuncsV2.cginc"


            ENDCG
        }

        Pass{
            Tags{
                "LightMode" = "ForwardAdd" // ForwardAdd makes it so this pass is "added" on top of the base one, used for the main light
            }
            // In this case Blend 0 One One is the same as Blend One One 
            // since this shader is not using the other targets.
            Blend 0 One One 
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
            
            #define HELIUM_NORMAL_MAPPING
            #define HELIUM_ADD_PASS
            #pragma shader_feature HELIUM_2D_METALLIC
            #pragma shader_feature HELIUM_2D_ROUGHNESS
            #pragma shader_feature HELIUM_PACKED_MR
            
            #pragma vertex vert
            #pragma fragment frag
            
			#include "LightingFuncsV2.cginc"
            ENDCG
            

        }
        Pass{
            Tags{
                "LightMode" = "ShadowCaster"
            }
            CGPROGRAM
            #pragma target 3.0
            #pragma vertex shadowVert
            #pragma fragment shadowFrag

            #pragma multi_compile_shadowcaster
            
            #include "ShadowFuncs.cginc"

            ENDCG
        }
    }
    Fallback "Diffuse"
    CustomEditor "HeliumShaderUI"
}
