Shader "Custom/GraphColorShader"
{
    Properties{
        _Smoothness("Smoothness",Range(0,1)) = 0.5
        _UChannel("UChannel",Range(0,1)) = 0.5
        _VChannel("VChannel",Range(0,1)) = 0.5
    }
    SubShader{
        CGPROGRAM
        #pragma surface ConfigureSurface Standard fullforwardshadows
        #pragma target 3.0

        struct Input{
            float3 worldPos;
        };

        float _Smoothness;
        float _UChannel;
        float _VChannel;

        void ConfigureSurface(Input input, inout SurfaceOutputStandard surface){
            surface.Smoothness = _Smoothness;
            surface.Albedo = input.worldPos*0.5 + fixed3(0.5,0.5,0.5);
            // surface.Albedo = fixed3(_UChannel,_VChannel, 0.0);
        }
        ENDCG
    }
    FallBack "Diffuse"
}
