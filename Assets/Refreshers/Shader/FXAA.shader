Shader "Refreshers/FXAA" {
	Properties {
		_MainTex ("Texture", 2D) = "white" {}
	}

	SubShader {
		Cull Off
		ZTest Always
		ZWrite Off

        CGINCLUDE
            #include "UnityCG.cginc"
            #define HELIUM_POSTPROCESS
            #include "HeliumMath.cginc"

            sampler2D _MainTex;
            float4 _MainTex_TexelSize;

            struct vIn{
                float4 pos : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct vOut{
                float4 pos : POSITION;
                float2 uv : TEXCOORD0;
            };

            vOut vert(vIn i){
                vOut o;
                o.pos = UnityObjectToClipPos(i.pos);
                o.uv = i.uv;
                return o;
            }
        ENDCG

		Pass {
            Name "Transfer"
			CGPROGRAM

            #pragma vertex vert
            #pragma fragment frag

            #define LUMA(c) Luminance(c);
            
            float4 frag(vOut i): SV_Target{
                float4 c = tex2D(_MainTex, i.uv);
                return LUMA(c);
            }
			ENDCG 
		}
	}
}