#ifndef HELIUM_TESSELATION
#define HELIUM_TESSELATION

float _Subdivs;
float _TargetEdgeLen;

struct vTesselatedOutput {
	float4 vertex : INTERNALTESSPOS;
	float3 n : TEXCOORD0;
	float4 uv : TEXCOORD1;

    #ifndef HELIUM_SIMPLIFIED_TESSELATION_STRUCT
        float4 tan : TEXCOORD3;
        #ifdef VERTEXLIGHT_ON
        float3 lColor: TEXCOORD6; // Computed vertex light
        #elif defined(LIGHTMAP_ON) || defined(HELIUM_MULITPLE_DIRECTIONAL_SHADOWMASKS)
        float2 uvLight : TEXCOORD6;
        #endif
        #ifdef DYNAMICLIGHTMAP_ON
        float2 uvDynLight : TEXCOORD7;
        #endif
    #endif
};

struct tessInfo{ // Amount of tesselation per patch edge and per inner surface
    float edge[3] : SV_TessFactor; 
    float inner : SV_InsideTessFactor;
};

float getSubdivs(
    vTesselatedOutput patch1, vTesselatedOutput patch2
){
    #ifdef HELIUM_EDGE_BASED_TESSELATION
    float3 p0 = mul(unity_ObjectToWorld, float4(patch1.vertex.xyz, 1)).xyz;
    float3 p1 = mul(unity_ObjectToWorld, float4(patch2.vertex.xyz, 1)).xyz;
    float len = distance(p0,p1);
    return len/ _TargetEdgeLen;
    #elif defined(HELIUM_SCREEN_SIZE_TESSELATION)
    float4 cs0 = UnityObjectToClipPos(patch1.vertex);
    float4 cs1 = UnityObjectToClipPos(patch2.vertex);
    float3 p0 = cs0.xyz / cs0.w;
    float3 p1 = cs1.xyz / cs1.w;
    return (distance(p0,p1) * _ScreenParams.y) / (2 * _TargetEdgeLen);
    #elif defined(HELIUM_SCREEN_DISTANCE_TESSELATION)
    float3 ws0 = mul(unity_ObjectToWorld, float4(patch1.vertex.xyz, 1)).xyz;
    float3 ws1 = mul(unity_ObjectToWorld, float4(patch2.vertex.xyz, 1)).xyz;
    float el = distance(ws0,ws1);
    float3 ec = ( ws0 + ws1 ) * 0.5;
    float d = distance(ec,_WorldSpaceCameraPos);
    return el / d * _TargetEdgeLen;
    #else
    return _Subdivs;
    #endif
}

tessInfo patchCutEval (InputPatch<vTesselatedOutput,3> p){
    _Subdivs = max(_Subdivs,1);
    _TargetEdgeLen = max(_TargetEdgeLen, 0.1);

    tessInfo ti;
    ti.edge[0] = getSubdivs(p[1],p[2]);
    ti.edge[1] = getSubdivs(p[2],p[0]);
    ti.edge[2] = getSubdivs(p[0],p[1]);
    
    ti.inner = (ti.edge[0]+ti.edge[1] + ti.edge[2]) / 3.0;
    return ti;
}

[UNITY_domain("tri")] // Work on tris (instead of quads or isoline)
[UNITY_outputcontrolpoints(3)] // Tesselation emits 3 points 
[UNITY_outputtopology("triangle_cw")] // Clockwise (unity standard for front )
[UNITY_partitioning("fractional_odd")] // force odd partitioning
[UNITY_patchconstantfunc("patchCutEval")] // establishes how many cuts for the current patch
vTesselatedOutput hull(InputPatch<vTesselatedOutput, 3> p, uint id: SV_OutputControlPointID){
    return p[id];
}

#define BARYCENTRIC_INTERP(p, var, field) var.field = p[0].field * bc.x + p[1].field * bc.y + p[2].field * bc.z;

[UNITY_domain("tri")] 
vOutput dom(
    tessInfo ti,
    OutputPatch<vTesselatedOutput, 3> p,
    float3 bc : SV_DomainLocation
){
    vInput vi;
    #ifndef HELIUM_SIMPLIFIED_TESSELATION_STRUCT
        BARYCENTRIC_INTERP(p, vi, tan);
        #if defined(LIGHTMAP_ON) || defined(HELIUM_MULITPLE_DIRECTIONAL_SHADOWMASKS)
        BARYCENTRIC_INTERP(p, vi, uvLight);
        #endif
        #ifdef DYNAMICLIGHTMAP_ON
        BARYCENTRIC_INTERP(p, vi, uvDynLight);
        #endif
    #endif
    // Weighted average based on barycentric coordinates
    // vi.vertex = p[0].vertex * bc.x + p[1].vertex * bc.y + p[2].vertex * bc.z;
    BARYCENTRIC_INTERP(p, vi, vertex);
    BARYCENTRIC_INTERP(p, vi, n);
    BARYCENTRIC_INTERP(p, vi, uv);
    return vert(vi);
}

vTesselatedOutput vertForTesselation(vInput v){
    vTesselatedOutput vtp;
    
    #ifndef HELIUM_SIMPLIFIED_TESSELATION_STRUCT
        vtp.tan = v.tan; 
        #if defined(LIGHTMAP_ON) || defined(HELIUM_MULITPLE_DIRECTIONAL_SHADOWMASKS)
        vtp.uvLight = v.uvLight;
        #endif
        #ifdef DYNAMICLIGHTMAP_ON
        vtp.uvDynLight = v.uvDynLight;
        #endif
    #endif
    vtp.uv = 0;
    vtp.uv.xy = v.uv;
    vtp.vertex = v.vertex ;
    vtp.n = v.n;
    return vtp;
}

#endif