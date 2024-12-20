Shader "Fractal/FractalShaderGPU"
{
    Properties{
        _Albedo("AlbedoA", Color) = (1.0,1.0,1.0,1.0)
        _Albedo("AlbedoB", Color) = (1.0,1.0,1.0,1.0)
        _Offset("Offset", Vector) = (0.381,0.381,0,0)
        _Smoothness("Smoothness",Range(0,1)) = 0.5
    }
    /*
    
    
    
    ================================ COPIED FROM GraphCSBuffer ================================
    Comments might be obsolete due to copy paste :) 
    
    
    */
    SubShader{
        CGPROGRAM
// Upgrade NOTE: excluded shader from OpenGL ES 2.0 because it uses non-square matrices
#pragma exclude_renderers gles
        // By default the procedural function will only get invoked for the regular draw pass. 
        // To have shadows being rendered we have to indicate that we need a custom shadow pass
        // addshadow is needed to have an extra shadow pass that calls ConfigurePosition.
        #pragma surface ConfigureSurface Standard fullforwardshadows addshadow 
        // Do not use JIT shader compilation. This might cause fallback to a cyan shader that is very slow.
        // Ask Unity to compile this synchronously immediately compile the shader right before it gets used the first time, avoiding the dummy shader.
        // This will stall Unity until the shader is compiled. (Not a problem for a compiled game)
        #pragma editor_sync_compilation
        #pragma target 4.5
        // tells the shader to use procedural rendering, by leveraging a function called "ConfigurePosition"
        // will call ConfigureProcedural for each vertex
        // We also use assumeuniformscaling in order to tell the shader not to care about correcting normal vectors, as the scale will be uniform across the entire scale.
		#pragma instancing_options procedural:ConfigurePosition assumeuniformscaling 

        // We can only run this if procedural instancing is enabled, this flag coming from unity allows us to check for it. 
        #if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
        StructuredBuffer<float3x4> _Matrices;
        #endif

        struct Input{
            float3 worldPos;
        };

        float4 _AlbedoA;
        float4 _AlbedoB;
        float _Smoothness;
        float2 _Offset;

        float4 DeriveElementAlbedo(){
            #if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
            // any scalar x is automatically converted to a float4(x,x,x,x)
            return lerp(_AlbedoA,_AlbedoB,frac(unity_InstanceID * _Offset.x + _Offset.y));
            #else
            return _AlbedoA;
            #endif
        }

        void ConfigurePosition () {
            #if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
            // unity_InstanceID is a value containing the ID of the instance being drawn 
            // by Graphics.RenderMeshPrimitives in GPUGraph.cs / Fractal.cs
            float3x4 mat = _Matrices[unity_InstanceID];
            unity_ObjectToWorld._m00_m01_m02_m03 = mat._m00_m01_m02_m03;
            unity_ObjectToWorld._m10_m11_m12_m13 = mat._m10_m11_m12_m13;
            unity_ObjectToWorld._m20_m21_m22_m23 = mat._m20_m21_m22_m23;
            unity_ObjectToWorld._m30_m31_m32_m33 = float4(0.0,0.0,0.0,1.0);
            // We also should transform the inverse matrix, in order to transform normals correctly. 
            // There is also a unity_WorldToObject matrix, which contains the inverse transformation.
            // We instead use "assumeuniformscaling" options for instancing, as in this case scaling will be uniform (same for all instances)
			#endif
        }

        void ConfigureSurface(Input input, inout SurfaceOutputStandard surface){
            surface.Smoothness = _Smoothness;
            surface.Albedo = DeriveElementAlbedo().rgb;
        }
        ENDCG
    }
    FallBack "Diffuse"
}