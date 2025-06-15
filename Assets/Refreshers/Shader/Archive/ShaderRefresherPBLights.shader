// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "Refreshers/MultiLightPhysicallyBased"
{
    Properties{
        _Tex ("Texture 1", 2D) = "white" {}
        _Roughness("Roughness", Range(0,1)) = 0.5
        _Metallic("Metallic", Range(0,1)) = 0.1
    }
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
            
            #pragma multi_compile_fwdadd // equivalent of the following
            // #pragma multi_compile DIRECTIONAL POINT SPOT DIRECTIONAL_COOKIE POINT_COOKIE

			#include "LightingFuncs.cginc"


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
            #pragma multi_compile_fwdadd
            
            #define HELIUM_ADD_PASS
            
            #pragma vertex vert
            #pragma fragment frag
            
			#include "LightingFuncs.cginc"
            ENDCG
            

        }
    }
    Fallback "Diffuse"
}
