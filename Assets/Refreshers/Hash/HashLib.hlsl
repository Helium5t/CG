
// Do not use JIT shader compilation. This might cause fallback to a cyan shader that is very slow.
// Ask Unity to compile this synchronously immediately compile the shader right before it gets used the first time, avoiding the dummy shader.
// This will stall Unity until the shader is compiled. (Not a problem for a compiled game)
#pragma editor_sync_compilation
// tells the shader to use procedural rendering, by leveraging a function called "ConfigurePosition"
// will call ConfigureProcedural for each vertex
// We also use assumeuniformscaling in order to tell the shader not to care about correcting normal vectors, as the scale will be uniform across the entire scale.
#pragma instancing_options assumeuniformscaling procedural:ConfigureProcedural

#if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
    StructuredBuffer<uint> _Hashes;
    StructuredBuffer<float3> _Coords;
    StructuredBuffer<float3> _Normals;
#endif 

// x = size of one side of the square = resolution
// y = size of one square (one side) = 1/resolution
// z = displacement/ resolution
float4 _ResolutionInfo;


void ConfigureProcedural(){
    #if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
        uint hash = _Hashes[unity_InstanceID];
        float3 coord = _Coords[unity_InstanceID];
        float heightOffset = (float)(hash >> 24)/(float)255.0;

        /* Previous planar offset
           v = row = 0*resolution, 1*resolution, 2*resolution ... v * resolution ... resolution * resolution
           float v = floor( _ResolutionInfo.y *  unity_InstanceID + 0.0001 ); // floor(instanceID / resolution) = 
           u = column
           float u = unity_InstanceID - _ResolutionInfo.x * v;
        */
        // init empty otw
		unity_ObjectToWorld = 0.0;
        unity_ObjectToWorld._m03_m13_m23_m33 = float4(coord, 1.0);
        // Here still offset height by third hash value
        unity_ObjectToWorld._m03_m13_m23 +=  (_ResolutionInfo.z * (heightOffset - 0.5)) * _Normals[unity_InstanceID];

        /* Previous planar offset
        // offset by resolution slot position
		unity_ObjectToWorld._m03_m13_m23_m33 = float4(
			_ResolutionInfo.y * (u + 0.5) - 0.5,
            _ResolutionInfo.z * heightOffset,
			_ResolutionInfo.y * (v + 0.5) - 0.5,
			1.0
		);
        */
        // scale by 1/resolution = square size
		unity_ObjectToWorld._m00_m11_m22 = _ResolutionInfo.y;
    #endif 
}

float3 GetHashColor () {
	#if defined(UNITY_PROCEDURAL_INSTANCING_ENABLED)
    uint hash = _Hashes[unity_InstanceID];
    return (1.0 / 255.0)  * float3(
        hash & 255,
        (hash >> 8) & 255,
        (hash >> 16) & 255
    );
    #else
    return 0.5;
    #endif
}