// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "Refreshers/BasicBlinnPhong"
{
    Properties{
        _Color("Color", Color) = (1,1,1,1)
        _Tex ("Texture 1", 2D) = "white" {}
        _Tex2 ("Texture 2", 2D) = "white" {}
        _Tex3 ("Texture 3", 2D) = "white" {}
        _Mask ("Mask Map", 2D) = "white"{}
        _UseTextures("Use Textures", Integer) = 0
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

            sampler2D _Tex, _Tex2,_Tex3, _Mask;
            float4 _Tex_ST, _Tex2_ST, _Tex3_ST, _Mask_ST;

            float4 _Color;
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
                float2 uvS : TEXCOORD2; // Second
                float2 uvT : TEXCOORD3; // Third
                float2 uvL : TEXCOORD4; // Lerp (Mask)
                float4 wPos : TEXCOORD5; // Worldspace Position
            };

            vOutput vert(vInput i){
                vOutput o;

                // o.uv = i.uv * _Tex_ST.xy;
                // o.uv += _Tex_ST.zw;
                o.uvM = TRANSFORM_TEX(i.uv, _Tex);  // QOL command that summarizes lines above
                o.uvS = HELIUM_TRANSFORM_TEX(i.uv, _Tex2);  // Same as Unity's TRANSFORM_TEX
                o.uvT = HELIUM_TRANSFORM_TEX(i.uv, _Tex3);
                o.uvL = HELIUM_TRANSFORM_TEX(i.uv, _Mask);
                o.csPos = UnityObjectToClipPos(i.pos);
                o.wPos = mul(unity_ObjectToWorld, i.pos);
                // Two ways of doing the same thing
                // o.n = mul(transpose((float3x3)unity_WorldToObject),i.n);
                // Instead of running transpose() UnityObjectToWorldNormal 
                //uses explicits transformations and is thus more optimized
                o.n = UnityObjectToWorldNormal(i.n); 
                o.n = normalize(o.n);
                return o;
            }

            float4 frag(vOutput vo): SV_Target{
                float4 sampleOne =  tex2D(_Tex, vo.uvM);
                float4 sampleTwo = tex2D(_Tex2, vo.uvS);
                float4 sampleThree = tex2D(_Tex3, vo.uvT);
                float4 mask = tex2D(_Mask, vo.uvL);
                float4 b = mask.r + mask.g + mask.b;
                float4 finalAlbedo = 
                    mask.r * sampleOne +
                    mask.g * sampleTwo + 
                    mask.b * sampleThree +
                    (1-step(0.1,b))* _Color;
                finalAlbedo =   step(0.5,_UseTextures) * finalAlbedo +1 * (1-step(0.5,_UseTextures)) ;
            
                // finalAlbedo.xyz *= (1- max(_SpecularColor.r, max(_SpecularColor.g, _SpecularColor.b))); // Energy conservation to avoid color > 1
                float invertedReflectivity;
                float3 specularColor;
                
                // specularColor and invertedReflectivity are out parameters
                finalAlbedo = float4(DiffuseAndSpecularFromMetallic(
					finalAlbedo.xyz, _Metallic, specularColor, invertedReflectivity
				),1.0); // Same as line 105

                // interpolating between unit vectors yields |v|<1 vectors.
                // float3 diff = normalize(vo.n) - vo.n; // difference is negligible so we won't optimize, it's only visible 20x 
                // Clamp to avoid negative values
                // float4 lightIntensity = clamp(dot(_WorldSpaceLightPos0.xyz, vo.n),0,1); // method 1 
                // float4 lightIntensity = saturate(dot(_WorldSpaceLightPos0.xyz, vo.n)); // method 2 saturate(x) = clamp(x,0,1)
                
                vo.n = normalize(vo.n);
				float3 lightDir = _WorldSpaceLightPos0.xyz;
                // _WorldSpaceLightPos0: Light 0 is a directional light so _WorldSpaceLightPos0
                // represents its direction as position is not relevant to computation
                float3 lightIntensity = DotClamped(lightDir, vo.n); // method 3 DotClamped(x,y) = saturate(dot(x,y))
                float4 finalDiffuse =  float4(lightIntensity,1.0) * _LightColor0 * finalAlbedo;




                // Basic Reflections
                float3 viewdir = normalize(_WorldSpaceCameraPos - vo.wPos);
                // float3 reflectionDir = reflect(-lightDir, vo.n); // Phong
                float3 reflectionDir = normalize(lightDir + viewdir); // Blinn-Phong
                float3 reflectedIntensity = pow( DotClamped( vo.n, reflectionDir), (1-_Roughness) *100);
                float4 finalSpecular =  _LightColor0 * float4(specularColor,1.0) *  float4(reflectedIntensity,1.0);
                return float4(finalSpecular.xyz + finalDiffuse.xyz, 1.0);
                // return finalDiffuse * finalAlbedo  /*Corrects linear to gamma transformation*/;
            }

            ENDCG
        }
    }
    Fallback "Diffuse"
}
