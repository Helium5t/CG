Shader "Refreshers/DeferredLight"
{
    Properties
    {
    }
    SubShader
    {
        // No culling or depth

        ZWrite Off 
        Pass
        {
            Name "Light Rendering"
            Blend [_SrcBlend] [_DstBlend] // Variable blend mode handled by unity depending on light used

            CGPROGRAM

            #pragma vertex vert
            #pragma fragment frag
            #pragma target 3.0
            #pragma exclude_renderers nomrt // No multiple render targets. This shader won't be included for platforms that do not support deferred rendering. 

			#pragma multi_compile_lightpass
			#pragma multi_compile _ UNITY_HDR_ON

            
            #include "DeferredLighting.cginc"

            ENDCG
        }
        Pass
        {
            Name "LDR Pass : HDR Decoding" // If HDR is disabled unity encodes colors logarithmically, this decodes the log(color). 

            Stencil {
                Ref [_StencilNonBackground]
                ReadMask [_StencilNonBackground]
                Comp Equal
            }

            CGPROGRAM

            #pragma vertex vert
            #pragma fragment frag

            #pragma target 3.0
            #pragma exclude_renderers nomrt // No multiple render targets. This shader won't be included for platforms that do not support deferred rendering. 

            #include "UnityCG.cginc"

            struct vInput{
                float4 pos : POSITION;
                float2 screenUV : TEXCOORD0; // UVs stretching over the screen. (Implemented engine side as a huge quad with each corner matching the screen's. Same as Deferred Fog.)
            };

            struct vOutput{
                float4 cPos : SV_POSITION; // Clip space
                float2 screenUV : TEXCOORD0;
            };

            vOutput vert(vInput v){
                vOutput o;
                o.cPos = UnityObjectToClipPos(v.pos);
                o.screenUV = v.screenUV;
                return o;
            }

            sampler2D _LightBuffer; // Unity built-in var


            float4 frag (vOutput i) : SV_Target
            {
                float4 col =  -log2(tex2D(_LightBuffer, i.screenUV));
                return col;
            }

            ENDCG
        }
    }
}
