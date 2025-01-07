Shader "Noise/NoiseRenderer"
{
    Properties
    {
        _Smoothness("Smoothness", Range(0.0,1.0)) = 0.5
    }
    SubShader
    {
        CGPROGRAM
        #pragma target 4.5

        #pragma surface ConfigureSurface Standard fullforwardshadows addshadow 
        // Pragma directives will be ignored if coming from the include file unless you use #include_with_pragmas which for some reason does not work here
        #include "NoiseLib.hlsl"
    
        // Do not use JIT shader compilation. This might cause fallback to a cyan shader that is very slow.
        // Ask Unity to compile this synchronously immediately compile the shader right before it gets used the first time, avoiding the dummy shader.
        // This will stall Unity until the shader is compiled. (Not a problem for a compiled game)
        #pragma editor_sync_compilation
        // tells the shader to use procedural rendering, by leveraging a function called "ConfigurePosition"
        // will call ConfigureProcedural for each vertex
        // We also use assumeuniformscaling in order to tell the shader not to care about correcting normal vectors, as the scale will be uniform across the entire scale.
        #pragma instancing_options assumeuniformscaling procedural:ConfigureProcedural

        float _Smoothness;

        struct Input{
            float3 wpos;
        };

        void ConfigureSurface(Input input, inout SurfaceOutputStandard surface){
            surface.Smoothness = _Smoothness;
            surface.Albedo = float4(GetNoiseColor(),1.0);
        }
        ENDCG
    }
    FallBack "Diffuse"
}
