Shader "Custom/CubemapFromVertex"
{
    Properties
    {
        _Cubemap ("Cubemap", CUBE) = "_Skybox" {}
    }

    SubShader
    {
        Tags { "RenderType" = "Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            samplerCUBE _Cubemap;

            struct appdata
            {
                float4 vertex : POSITION; // Vertex position
            };

            struct v2f
            {
                float3 worldNormal : TEXCOORD0; // Pass world space normal
                float4 vertex : SV_Position; // Position for rasterization
            };

            v2f vert (appdata v)
            {
                v2f o;
                // Transform vertex position to clip space
                o.vertex = UnityObjectToClipPos(v.vertex);

                // Use the normalized vertex position as the direction for sampling the cubemap
                o.worldNormal = normalize(v.vertex.xyz);
                return o;
            }

            half4 frag (v2f i) : SV_Target
            {
                // Sample the cubemap using the worldNormal
                half4 cubemapColor = texCUBE(_Cubemap, i.worldNormal);
                return cubemapColor;
            }
            ENDCG
        }
    }

    FallBack "Diffuse"
}
