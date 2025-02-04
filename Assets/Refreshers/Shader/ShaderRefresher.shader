// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "Refreshers/AmbientShader"
{
    Properties{
        _Color("Color", Color) = (1,1,1,1)
        _Tex ("Texture 1", 2D) = "white" {}
        _Tex2 ("Texture 2", 2D) = "white" {}
        _Tex3 ("Texture 3", 2D) = "white" {}
        _Mask ("Mask Map", 2D) = "white"{}
    }
    SubShader{

        Pass{
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            
            // To prove TRANSFORM_TEX is just a define
            #define HELIUM_TRANSFORM_TEX(x,y) (x.xy * y##_ST.xy + y##_ST.zw)

            sampler2D _Tex, _Tex2,_Tex3, _Mask;
            float4 _Tex_ST, _Tex2_ST, _Tex3_ST, _Mask_ST;

            float4 _Color;

            struct vInput{
                float4 pos: POSITION;
                float3 n : NORMAL;
                float2 uv : TEXCOORD0;
            };

            struct vOutput{
                float4 wPos : SV_Position;
                float2 uvM : TEXCOORD0; // Main
                float2 uvS : TEXCOORD1; // Second
                float2 uvT : TEXCOORD2; // Third
                float2 uvL : TEXCOORD3; // Lerp (Mask)
            };

            vOutput vert(vInput i){
                vOutput o;

                // o.uv = i.uv * _Tex_ST.xy;
                // o.uv += _Tex_ST.zw;
                o.uvM = TRANSFORM_TEX(i.uv, _Tex);  // QOL command that summarizes lines above
                o.uvS = HELIUM_TRANSFORM_TEX(i.uv, _Tex2);  // Same as Unity's TRANSFORM_TEX
                o.uvT = HELIUM_TRANSFORM_TEX(i.uv, _Tex3);
                o.uvL = HELIUM_TRANSFORM_TEX(i.uv, _Mask);
                o.wPos = UnityObjectToClipPos(i.pos);
                return o;
            }

            float4 frag(vOutput vo): SV_Target{
                float4 sampleOne =  tex2D(_Tex, vo.uvM);
                float4 sampleTwo = tex2D(_Tex2, vo.uvS);
                float4 sampleThree = tex2D(_Tex3, vo.uvT);
                float4 mask = tex2D(_Mask, vo.uvL);
                float4 b = mask.r + mask.g + mask.b;
                float4 final = 
                    mask.r * sampleOne +
                    mask.g * sampleTwo + 
                    mask.b * sampleThree +
                    (1-step(0.1,b))* _Color;
                return final  /*Corrects linear to gamma transformation*/;
            }

            ENDCG
        }
    }
    Fallback "Diffuse"
}
