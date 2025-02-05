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
            // #include "UnityCG.cginc" // Already included by UnityStandardBRDF
            // #include "UnityStandardBRDF.cginc" // in UnityPBSLighting
            // For energy conservation checks on blinn-phong
            // #include "UnityStandardUtils.cginc" // in UnityPBSLighting 

			#include "LightingFuncs.cginc"

            ENDCG
        }
    }
    Fallback "Diffuse"
}
