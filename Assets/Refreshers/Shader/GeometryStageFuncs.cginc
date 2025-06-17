#ifndef HELIUM_GEOMETRY_FUNCS_INCLUDED
#define HELIUM_GEOMETRY_FUNCS_INCLUDED

#include "LightingFuncsV3.cginc"

// [maxvertexcount(3)]
void geo(triangle vOutput vos[3], inout TriangleStream<vOutput> ts){
    vos[0].wPos = 0;
    vos[0].n = 0;
    ts.Append(vos[0]);
    // float3 n = vos[0].n;
    // vos[0].n = -vos[1].n;
    // vos[1].n = -vos[2].n;
    // vos[2].n = -n;
    // ts.Append(vos[0]);
    // ts.Append(vos[1]);
    // ts.Append(vos[2]);
}

#endif