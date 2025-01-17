// Upgrade NOTE: replaced '_Object2World' with 'unity_ObjectToWorld'

Shader "ProcMesh/RippleAnimShader"
{
    Properties
    {
        _Color ("Color", Color) = (1,1,1,1)
        _MainTex ("Albedo (RGB)", 2D) = "white" {}
        _Normal("Normal",2D) = "bump"{}
        _Amplitude("Amplitude", Range(0,10)) = 0.3
        _Period("Period", Range(0,3)) = 0.3
        _Frequency("Frequency", Range(0,3)) = 0.3
        _Glossiness ("Smoothness", Range(0,1)) = 0.5
        _Metallic ("Metallic", Range(0,1)) = 0.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200

        CGPROGRAM
        // Physically based Standard lighting model, and enable shadows on all light types

        #pragma vertex vert
        #include "UnityCG.cginc"
        #pragma surface surf Standard fullforwardshadows
        #define PI 3.1415926535897

        // Use shader model 3.0 target, to get nicer looking lighting
        #pragma target 3.0

        sampler2D _MainTex;
        sampler2D _Normal;
        half _Amplitude;
        half _Frequency;
        half _Period;

        struct Input
        {
            float2 uv_MainTex;
        };

        void vert(inout appdata_full v){
            float2 c = v.texcoord.xy - 0.5;
            float l = length(c);
            float f = 2.0 * PI * _Period * (l -  _Frequency * _Time.y);
            v.vertex.xyz += v.normal * _Amplitude * cos(f);
        }

        half _Glossiness;
        half _Metallic;
        fixed4 _Color;

        // Add instancing support for this shader. You need to check 'Enable Instancing' on materials that use the shader.
        // See https://docs.unity3d.com/Manual/GPUInstancing.html for more information about instancing.
        // #pragma instancing_options assumeuniformscaling
        UNITY_INSTANCING_BUFFER_START(Props)
            // put more per-instance properties here
        UNITY_INSTANCING_BUFFER_END(Props)

        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            // Albedo comes from a texture tinted by color
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb;
            // Metallic and smoothness come from slider variables
            o.Metallic = _Metallic;
            o.Smoothness = _Glossiness;
            o.Alpha = c.a;
            o.Normal = UnpackNormal(tex2D(_Normal, IN.uv_MainTex));
        }
        ENDCG
    }
    FallBack "Diffuse"
}
