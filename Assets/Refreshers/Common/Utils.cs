using Unity.Mathematics;
using static Unity.Mathematics.math;

// using float3x3 = Unity.Mathematics.float3x3;
using float4x4 = Unity.Mathematics.float4x4;
using quaternion = Unity.Mathematics.quaternion;


public static class Util{
    public static float3x4 TRS(float3 pos, quaternion rot, float scale){
        float3x3 scaleRot = float3x3(rot) * scale;
        return float3x4(scaleRot.c0, scaleRot.c1, scaleRot.c2, pos);
    }
    public static float3x4 TRS(float3 pos, float3 rot, float3 scale){
        float4x4 m = float4x4.TRS(
            pos, quaternion.EulerZXY(radians(rot)), scale
        );
		return math.float3x4(m.c0.xyz, m.c1.xyz, m.c2.xyz, m.c3.xyz);
    }
}