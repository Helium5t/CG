// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "Refreshers/BasicPhysicallyBased"
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

			#include "UnityPBSLighting.cginc"
            
            // To prove TRANSFORM_TEX is just a define
            #define HELIUM_TRANSFORM_TEX(x,y) (x.xy * y##_ST.xy + y##_ST.zw)

            sampler2D _Tex;
            float4 _Tex_ST;

            float _Roughness, _Metallic;
            int _UseTextures;

            struct vInput{
                float4 pos: POSITION;
                float3 n : NORMAL;
                float2 uv : TEXCOORD0;
            };

            struct vOutput{
                float4 csPos : SV_Position; // Clip Space
                float3 n :   TEXCOORD0;
                float2 uvM : TEXCOORD1; // Main
                float4 wPos : TEXCOORD5; // Worldspace Position
            };

            vOutput vert(vInput i){
                vOutput o;

                o.uvM = HELIUM_TRANSFORM_TEX(i.uv, _Tex);  // QOL command that summarizes texture tiling and offset
                o.csPos = UnityObjectToClipPos(i.pos);
                o.wPos = mul(unity_ObjectToWorld, i.pos);

                o.n = UnityObjectToWorldNormal(i.n); 
                o.n = normalize(o.n);
                return o;
            }

            float4 frag(vOutput vo): SV_Target{
                float3 albedo =  tex2D(_Tex, vo.uvM);

                UnityLight l;
                l.color = _LightColor0;
                l.dir = _WorldSpaceLightPos0.xyz;
                // angle with surface normal
				l.ndotl = DotClamped(vo.n, _WorldSpaceLightPos0.xyz);

                UnityIndirect il;
                il.diffuse = 0;  // Ambient light
                il.specular = 0;  // Environmental reflections


                float invertedReflectivity;
                float3 specularColor;
                // specularColor and invertedReflectivity are out parameters
                albedo = DiffuseAndSpecularFromMetallic(
					albedo, _Metallic, specularColor, invertedReflectivity
				); 
                float3 viewdir = normalize(_WorldSpaceCameraPos - vo.wPos);
                
                vo.n = normalize(vo.n);
                return UNITY_BRDF_PBS(
                    albedo,
                    specularColor,
                    invertedReflectivity,
                    1.0-_Roughness,
                    vo.n,
                    viewdir,
                    l,
                    il
                );
                // return finalDiffuse * albedo  /*Corrects linear to gamma transformation*/;
            }

            ENDCG
        }
    }
    Fallback "Diffuse"
}
