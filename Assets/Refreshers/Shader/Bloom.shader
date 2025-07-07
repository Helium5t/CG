Shader "Refreshers/BloomPostProcess" {
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
            sampler2D _SourceTex;
            half4 _Bloom;

            struct vIn{
                float4 pos : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct vOut{
                float4 pos : POSITION;
                float2 uv : TEXCOORD0;
            };
        ENDCG

		Pass {
            Name "Downsample"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag
                
                vOut vert(vIn i){
                    vOut o;
                    o.pos = UnityObjectToClipPos(i.pos);
                    o.uv = i.uv;
                    return o;
                }

                half4 frag(vOut i): SV_Target{
                    half4 s;
                    BOX(i.uv,1,  s);
                    return s;
                }
			ENDCG
		}
		Pass {
            Blend One One
            Name "Upsample"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag

                vOut vert(vIn i){
                    vOut o;
                    o.pos = UnityObjectToClipPos(i.pos);
                    o.uv = i.uv;
                    return o;
                }

                half4 frag(vOut i): SV_Target{
                    half4 s;
                    BOX(i.uv,0.5,  s);
                    return s;
                }
			ENDCG
		}
		Pass {
            Name "Transfer"
			CGPROGRAM
                #include "UnityCG.cginc"
                #define HELIUM_POSTPROCESS
                #include "HeliumMath.cginc"

				#pragma vertex vert
				#pragma fragment frag

                vOut vert(vIn i){
                    vOut o;
                    o.pos = UnityObjectToClipPos(i.pos);
                    o.uv = i.uv;
                    return o;
                }

                half4 frag(vOut i): SV_Target{
                    half4 c = tex2D(_SourceTex, i.uv);
                    half4 s;
                    BOX(i.uv,0.5,s);
                    return c + (s * _Bloom.x);
                }
			ENDCG
		}
        Pass{
            Name "Prepass"
            CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag

                vOut vert(vIn i){
                    vOut o;
                    o.pos = UnityObjectToClipPos(i.pos);
                    o.uv = i.uv;
                    return o;
                }

                half4 frag(vOut i): SV_Target{
                    half4 c = tex2D(_MainTex, i.uv);
                    half b = max(c.r,max(c.g,c.b));
                    half s = clamp(b - _Bloom.y, 0, _Bloom.z);
                    s *= s * _Bloom.w;
                    half final = max(s, b - _Bloom.x);
                    final /= max(b, 0.00001);
                    return final * c;
                }
            ENDCG
        }
	}
}