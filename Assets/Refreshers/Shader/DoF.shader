Shader "Refreshers/DoF" {
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
            sampler2D _CocTex, _BlurTex;
            sampler2D _CameraDepthTexture;
            float4 _physCameraParams;

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
            Name "CoC"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag
                
                half frag(vOut i): SV_Target{
                    float d = LinearEyeDepth(SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, i.uv).r);
                    d =  (d - _physCameraParams.x) / _physCameraParams.y;
                    d = clamp(d,-1,1) * _physCameraParams.z;
                    return d;
                }
			ENDCG 
		}
		Pass {
            Name "Disc Blur"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag
                #define HELIUM_BLUR_KERNEL_MED
                

                // Kernels are from a unity library (Shaders/Builtins/DiskKernels.hlsl)
                // Array of kernels with len <= 1. Have to be multiplied to actually blr the image.
                #ifdef HELIUM_BLUR_KERNEL_SMALL
                static const int kernelValues = 16;
				static const float2 k[kernelValues] = {
					float2(0, 0),
					float2(0.54545456, 0),
					float2(0.16855472, 0.5187581),
					float2(-0.44128203, 0.3206101),
					float2(-0.44128197, -0.3206102),
					float2(0.1685548, -0.5187581),
					float2(1, 0),
					float2(0.809017, 0.58778524),
					float2(0.30901697, 0.95105654),
					float2(-0.30901703, 0.9510565),
					float2(-0.80901706, 0.5877852),
					float2(-1, 0),
					float2(-0.80901694, -0.58778536),
					float2(-0.30901664, -0.9510566),
					float2(0.30901712, -0.9510565),
					float2(0.80901694, -0.5877853),
				};
                #elif defined(HELIUM_BLUR_KERNEL_MED)
                static const int kernelValues = 22;
					static const float2 k[kernelValues] = {
						float2(0, 0),
						float2(0.53333336, 0),
						float2(0.3325279, 0.4169768),
						float2(-0.11867785, 0.5199616),
						float2(-0.48051673, 0.2314047),
						float2(-0.48051673, -0.23140468),
						float2(-0.11867763, -0.51996166),
						float2(0.33252785, -0.4169769),
						float2(1, 0),
						float2(0.90096885, 0.43388376),
						float2(0.6234898, 0.7818315),
						float2(0.22252098, 0.9749279),
						float2(-0.22252095, 0.9749279),
						float2(-0.62349, 0.7818314),
						float2(-0.90096885, 0.43388382),
						float2(-1, 0),
						float2(-0.90096885, -0.43388376),
						float2(-0.6234896, -0.7818316),
						float2(-0.22252055, -0.974928),
						float2(0.2225215, -0.9749278),
						float2(0.6234897, -0.7818316),
						float2(0.90096885, -0.43388376),
					};
                #endif

                half4 frag(vOut i): SV_Target{
                    half4 c;
                    half w = 0;
                    for (int v = 0; v < kernelValues; v++){
                        float2 t = k[v] *  _physCameraParams.z;
                        half sampleDist = length(t);
                        t *= _MainTex_TexelSize.xy;
                        half4 sample = COL(i.uv + t);
                        if(abs(sample.a) >= sampleDist){
                            c += sample;
                            w += 1; 
                        }
                    }
                    c *= 1.0 / w;
                    c.a = 1;
                    return c;
                }
			ENDCG
		}
		Pass {
            Name "Box Blur"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag

                half4 frag(vOut i): SV_Target{
                    half4 c;
                    BOX(i.uv,0.5,c);
                    return c ;
                }
			ENDCG
		}
		Pass {
            Name "CoC Scale"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag

                half4 frag(vOut i): SV_Target{
                    float4 offset = _MainTex_TexelSize.xyxy * float2(-0.5,0.5).xxyy;
                    half4 c;
                    c.r = tex2D(_CocTex, i.uv + + offset.xy).r;
                    c.g = tex2D(_CocTex, i.uv + + offset.xw).r;
                    c.b = tex2D(_CocTex, i.uv + + offset.zy).r;
                    c.a = tex2D(_CocTex, i.uv + + offset.zw).r;
                    half cmx = max(max(max(c.r,c.g),c.b),c.a);
                    c.r = min(min(min(c.r,c.g),c.b),c.a);
                    float l = step(-c.r, cmx);
                    c.r = cmx >= -c.r ? cmx : c.r; 
                    return half4(COL(i.uv).rgb, c.r) ;
                }
			ENDCG
		}
		Pass {
            Name "Final"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag


                half4 frag(vOut i): SV_Target{
                    float l = tex2D(_CocTex, i.uv) ;
                    l = ((l+1) * step(l,0)) + ((1-l) * step(0,l));
                    half4 c = lerp(tex2D(_BlurTex, i.uv), tex2D(_MainTex, i.uv), l);
                    return c;
                }
			ENDCG
		}
		Pass {
            Name "DebugFocus"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag


                half4 frag(vOut i): SV_Target{
                    float d = LinearEyeDepth(SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, i.uv).r);
                    // return clamp(d / _physCameraParams.x;
                    d = clamp(d - _physCameraParams.x, -_physCameraParams.y, _physCameraParams.y);
                    d /=  _physCameraParams.y;
                    return ((d+1) * step(d,0)) + ((1-d) * step(0,d));
                    // return clamp(d - _physCameraParams.x +_physCameraParams.y , 0 , 2 * _physCameraParams.y)/ (2* _physCameraParams.y);
                }
			ENDCG
		}
	}
}