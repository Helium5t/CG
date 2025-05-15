Shader "Refreshers/Postprocess Deferred Fog" {
	
	Properties {
        // Mandatory usage of MainTex otherwise unity does not write the input image to it
		_MainTex ("Source", 2D) = "white" {}
	}

	SubShader {
		Cull Off
		ZTest Always
		ZWrite Off

		Pass {
            CGPROGRAM
// Upgrade NOTE: excluded shader from DX11 because it uses wrong array syntax (type[size] name)
#pragma exclude_renderers d3d11
            
            #pragma vertex mapValues
            #pragma fragment post_process

            #pragma multi_compile_fog
            #if !defined(FOG_LINEAR) && !defined(FOG_EXP) && !defined(FOG_EXP2)
                    #define HELIUM_FOG_DISABLED
            #else
                    #define HELIUM_FOG_USE_WORLD_SPACE_DISTANCE
                    #define HELIUM_FOG_SHOW_SKYBOX
            #endif
            #include "UnityCG.cginc"
            
            sampler2D _MainTex;
            #ifndef HELIUM_FOG_DISABLED
            sampler2D _CameraDepthTexture;
            #endif
            float3 _CameraFarCorners[4]; // Written as 0 = BL, 1 = TL, 2 = TR, 3 = BR

            struct vertData{
                float4 vertex : POSITION;
                float2 uv: TEXCOORD0;
            };
            
            struct ppInput{
                float4 pos : SV_POSITION;
                float2 uv : TEXCOORD0;
                #ifdef HELIUM_FOG_USE_WORLD_SPACE_DISTANCE
                    float3 farPlaneRay : TEXCOORD1;
                #endif
            };


            ppInput mapValues(vertData i){
                ppInput o;
                o.pos = UnityObjectToClipPos(i.vertex);
                // When usig the Graphics.Blit, we access the vertex of a fake quad the size of the entire screen that has the destination texture mapped to.
                // Thus, the UV will be, for each run of the vertex shader, the corners of this quad. 
                // The UV of the corners will be BL = (0,0), BR = (1,0), TL = (0.1), TR = (1,1)
                o.uv = i.uv; 
                #ifdef HELIUM_FOG_USE_WORLD_SPACE_DISTANCE
                    o.farPlaneRay = _CameraFarCorners[i.uv.x + 2 * i.uv.y];
                #endif
                return o;
            }

            float4 post_process(ppInput i) : SV_Target{
                #ifndef HELIUM_FOG_DISABLED
                    float d = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, i.uv); //SAMPLE_DEPTH_TEXTURE because sampling is platofrm dependent (e.g. depth is 1-0 or 0-1 for closest to furthest, uv space y can be top down or bottom up etc...)
                    d = Linear01Depth(d); // The reason why we do this is because depth is not stored linearly, more bits are prioritized for closer objects, 
                    #ifdef HELIUM_FOG_USE_WORLD_SPACE_DISTANCE
                    float dist = length(i.farPlaneRay * d);
                    #else
                    float dist = d * _ProjectionParams.z - _ProjectionParams.y;
                    #endif
                    UNITY_CALC_FOG_FACTOR_RAW(dist); // Outputs unityFogFactor
				    unityFogFactor = saturate(unityFogFactor);
                    #ifdef HELIUM_FOG_SHOW_SKYBOX
                    float skyboxLerp = step(0.999f, d);
                    unityFogFactor = lerp(unityFogFactor,1, skyboxLerp);
                    #endif

                    float3 col = lerp(unity_FogColor.rgb,tex2D(_MainTex, i.uv).rgb, unityFogFactor);
                #else
                    float3 col = tex2D(_MainTex, i.uv).rgb;
                #endif
                return float4(col , 1);
            }

            ENDCG
		}
	}
}