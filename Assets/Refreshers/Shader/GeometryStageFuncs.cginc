#ifndef HELIUM_GEOMETRY_FUNCS_INCLUDED
#define HELIUM_GEOMETRY_FUNCS_INCLUDED

float3 _WFColor;
float _WireframeSmoothing;
float _WireframeThickness;

#include "LightingFuncsV3.cginc"

struct geoOutput{
    vOutput vo;   
    float2 baryCoord : TEXCOORD9;
};


[maxvertexcount(3)]
void geo (
	triangle vOutput inp[3],
	inout TriangleStream<geoOutput> ts
) {
    #ifdef HELIUM_FLAT_SHADING
    float3 p0 = inp[0].wPos.xyz;
	float3 p1 = inp[1].wPos.xyz;
	float3 p2 = inp[2].wPos.xyz;
    float3 triangleNormal = normalize(cross(p1 - p0, p2 - p0));
    inp[0].n = triangleNormal;
	inp[1].n = triangleNormal;
	inp[2].n = triangleNormal;
    #endif

    geoOutput g0, g1, g2;
    g0.vo = inp[0];
    g1.vo = inp[1];
    g2.vo = inp[2];
    g0.baryCoord = float2(1,0);
    g1.baryCoord = float2(0,1);
    g2.baryCoord = float2(0,0);
    ts.Append(g0);
	ts.Append(g1);
	ts.Append(g2);
}
#endif