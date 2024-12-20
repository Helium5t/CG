Shader "Custom/GraphCSBuffer"
{
    Properties{
        _Smoothness("Smoothness",Range(0,1)) = 0.5
        _UChannel("UChannel",Range(0,1)) = 0.5
        _VChannel("VChannel",Range(0,1)) = 0.5
    }
    SubShader{
        CGPROGRAM
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
        // Same as _Positions as in GraphColorShader.shader
        // but not writeable so StructuredBuffer instead of RWStructuredBuffer.
        StructuredBuffer<float3> _Positions;
        #endif

        struct Input{
            float3 worldPos;
        };

        float _Smoothness;
        float _UChannel;
        float _VChannel;

        float _CubeSize;

        void ConfigurePosition () {
            #if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
            // unity_InstanceID is a value containing the ID of the instance being drawn 
            // by Graphics.RenderMeshPrimitives in GPUGraph.cs
            float3 position = _Positions[unity_InstanceID];
            unity_ObjectToWorld = 0.0;
            // Accesses [0,3][1,3][2,3], thus setting position (offset)
            /*
                [][][][p.x]
                [][][][p.y]
                [][][][p.z]
                [][][][1.0]
            Increasing [3,3] would scale the entire object inversely proportional to the value => 1.5 scales down, 0.5 scales up.
            */
            unity_ObjectToWorld._m03_m13_m23_m33 = float4(position,1.0);
            unity_ObjectToWorld._m00_m11_m22 = _CubeSize;

            // We also should transform the inverse matrix, in order to transform normals correctly. 
            // There is also a unity_WorldToObject matrix, which contains the inverse transformation.
            // We instead use "assumeuniformscaling" options for instancing, as in this case scaling will be uniform (same for all instances)
			#endif
        }

        void ConfigureSurface(Input input, inout SurfaceOutputStandard surface){
            surface.Smoothness = _Smoothness;
            surface.Albedo = input.worldPos*0.5 + fixed3(0.5,0.5,0.5);
            // surface.Albedo = fixed3(_UChannel,_VChannel, 0.0);
        }
        ENDCG
    }
    FallBack "Diffuse"
}