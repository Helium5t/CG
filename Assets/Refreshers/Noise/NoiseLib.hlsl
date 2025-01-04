
// Do not use JIT shader compilation. This might cause fallback to a cyan shader that is very slow.
// Ask Unity to compile this synchronously immediately compile the shader right before it gets used the first time, avoiding the dummy shader.
// This will stall Unity until the shader is compiled. (Not a problem for a compiled game)
#pragma editor_sync_compilation
// tells the shader to use procedural rendering, by leveraging a function called "ConfigurePosition"
// will call ConfigureProcedural for each vertex
// We also use assumeuniformscaling in order to tell the shader not to care about correcting normal vectors, as the scale will be uniform across the entire scale.
#pragma instancing_options assumeuniformscaling procedural:ConfigureProcedural

#if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
    StructuredBuffer<uint> _Noises;
    StructuredBuffer<float3> _Coords;
    StructuredBuffer<float3> _Normals;
#endif 

// x = size of one side of the square = resolution
// y = size of one square (one side) = 1/resolution
float4 _ResolutionInfo;


void ConfigureProcedural(){
    #if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
        float3 coord = _Coords[unity_InstanceID];

        // init empty otw
		unity_ObjectToWorld = 0.0;
        unity_ObjectToWorld._m03_m13_m23_m33 = float4(coord, 1.0);

        unity_ObjectToWorld._m03_m13_m23 +=  (_ResolutionInfo.z * _Noise[unity_InstanceID]) * _Normals[unity_InstanceID];
        // scale by 1/resolution = square size
		unity_ObjectToWorld._m00_m11_m22 = _ResolutionInfo.y;
    #endif 
}

float3 GetNoiseColor() {
	#if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
    float noise = _Noises[unity_InstanceID];
    // Redscale when value is negatvie, otherwise greyscale.
    return noise < 0.0 ? float3( -noise, 0.0,0.0): noise;
    #else
    return 1.0;
    #endif
}