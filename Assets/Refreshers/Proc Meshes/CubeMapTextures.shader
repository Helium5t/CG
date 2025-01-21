Shader "ProcMesh/CubeMapTextures"
{
    Properties
    {
        _Color ("Color", Color) = (1,1,1,1)
        _MainTex ("Albedo (RGB)", Cube) = "white" {}
        _Normal ("Normal (Object Space)", Cube) = "bump" {}
        _Glossiness ("Smoothness", Range(0,1)) = 0.5
        _Metallic ("Metallic", Range(0,1)) = 0.0
    }
    SubShader{
        Tags { "RenderType"="Opaque" }
        LOD 200

        CGPROGRAM

        #pragma vertex vert
        #include "UnityCG.cginc"
        // Physically based Standard lighting model, and enable shadows on all light types
        #pragma surface surf Standard fullforwardshadows

        // Use shader model 3.0 target, to get nicer looking lighting
        #pragma target 3.0

        samplerCUBE _MainTex;
        samplerCUBE _Normal;

        struct Input
        {
            // float2 uv_MainTex;
            float3 normal;
        };

        half _Glossiness;
        half _Metallic;
        fixed4 _Color;

        void vert(inout appdata_full v, out Input IN){
            IN.normal = v.vertex;

        }
        // Add instancing support for this shader. You need to check 'Enable Instancing' on materials that use the shader.
        // See https://docs.unity3d.com/Manual/GPUInstancing.html for more information about instancing.
        // #pragma instancing_options assumeuniformscaling
        UNITY_INSTANCING_BUFFER_START(Props)
            // put more per-instance properties here
        UNITY_INSTANCING_BUFFER_END(Props)

        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            // Metallic and smoothness come from slider variables
            o.Metallic = _Metallic;
            o.Smoothness = _Glossiness;
            
            fixed4 c = texCUBE(_MainTex, IN.normal) * _Color;
            // o.Normal =   UnpackNormal(tex2D(_Normal, IN.uv_MainTex));
            float4 texNormal = normalize(texCUBE(_Normal,IN.normal));
            float3 vTangent = abs(o.Normal.z) < 0.999 ? float3(-o.Normal.y, o.Normal.x, 0):float3(0,-o.Normal.z, o.Normal.y);
            float3 bitangent = cross(texNormal, vTangent);
            float3x3 transformMatrix = float3x3(vTangent, bitangent, o.Normal);
            float3 t = mul(transformMatrix , texNormal.xyz);
            float3 fNormal = o.Normal.xyz + t; 
            o.Normal = normalize(fNormal);
            
            o.Albedo = c.rgb;
            o.Alpha = c.a;

        }
        ENDCG
    }
    FallBack "Diffuse"
}
