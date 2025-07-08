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
            half4 _physCameraParams;

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
                    float d = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, i.uv);
                    d = LinearEyeDepth(d);
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

                #define WEIGHT_F(s,d) saturate((s - d +2) / 2)

                half4 frag(vOut i): SV_Target{
                    half coc = tex2D(_MainTex, i.uv).a;

                    half4 cBg, cFg;
                    half totalBgW =0, totalFgW = 0;
                    for (int v = 0; v < kernelValues; v++){
                        half2 t = k[v] *  _physCameraParams.z;
                        half sampleDist = length(t);
                        t *= _MainTex_TexelSize.xy;
                        half4 sample = COL(i.uv + t);

                        half sampleWBG = WEIGHT_F(max(0, min(sample.a, coc)), sampleDist);
                        cBg.rgb += sample.rgb * sampleWBG;
                        totalBgW += sampleWBG; 

                        half sampleWFG = WEIGHT_F(-sample.a, sampleDist);
                        cFg.rgb += sample.rgb * sampleWFG;
                        totalFgW += sampleWFG; 
                    }
                    cBg *= 1.0 / (totalBgW + (totalBgW == 0));
                    cFg *= 1.0 / (totalFgW + (totalFgW == 0));
                    cBg.a = 1;
                    cFg.a = 1;
                    half focusLerp = min(1, totalFgW * 3.14159265359 / kernelValues);
                    return  half4(lerp(cBg, cFg, focusLerp).rgb, focusLerp);
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

                #define WEIGHT_F(c) 1 / (1 + max(max(c.r, c.g), c.b));

                half4 frag(vOut i): SV_Target{
					float4 o = _MainTex_TexelSize.xyxy * float2(-0.5,0.5).xxyy;

					half3 s0 = tex2D(_MainTex, i.uv + o.xy).rgb;
					half3 s1 = tex2D(_MainTex, i.uv + o.zy).rgb;
					half3 s2 = tex2D(_MainTex, i.uv + o.xw).rgb;
					half3 s3 = tex2D(_MainTex, i.uv + o.zw).rgb;

					half w0 = WEIGHT_F(s0);
					half w1 = WEIGHT_F(s1);
					half w2 = WEIGHT_F(s2);
					half w3 = WEIGHT_F(s3);

                    half3 weightedCol = (s0 * w0 + s1 * w1 + s2 * w2 + s3 * w3) / max(w0 + w1 + w2 + w3, 0.00001);

                    half cocr     = tex2D(_CocTex, i.uv + o.xy).r;
					half cocg     = tex2D(_CocTex, i.uv + o.zy).r;
					half cocb     = tex2D(_CocTex, i.uv + o.xw).r;
					half coca     = tex2D(_CocTex, i.uv + o.zw).r;
				
					half cocMin = min(min(min(cocr, cocg), cocb), coca);
					half cocMax = max(max(max(cocr, cocg), cocb), coca);
					half coc = cocMax >= -cocMin ? cocMax : cocMin;
                    return half4(weightedCol,coc);
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
                    half4 blurCol = tex2D(_BlurTex, i.uv);
                    l = smoothstep(0.1,1, abs(l));
                    half4 c = lerp(tex2D(_MainTex, i.uv), blurCol, l + blurCol.a - l * blurCol.a);
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
		Pass {
            Name "ShowCOC"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag


                half4 frag(vOut i): SV_Target{
                    half d = COL(i.uv);
                    d /= _physCameraParams.z;

                    return lerp(
                        -half4(1,0,0,1) * d,
                        half4(0,0,1,1) * d,
                        step(0,d)
                    );
                }
			ENDCG
		}
		Pass {
            Name "ShowCOCScale"
			CGPROGRAM

				#pragma vertex vert
				#pragma fragment frag


                half4 frag(vOut i): SV_Target{
                    half d = tex2D(_MainTex, i.uv).a;
                    // return d;
                    // d = tex2D(_CocTex, i.uv);
                    d /= _physCameraParams.z;

                    return lerp(
                        -half4(1,0,1,1) * d,
                        half4(0,1,1,1) * d,
                        step(0,d)
                    );
                }
			ENDCG
		}
	}
}