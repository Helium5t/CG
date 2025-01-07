using Unity.Mathematics;

using static Unity.Mathematics.math;

public static class MathLib{
    // From tutorial, supports float3 vectors
    public static float4x3 TransformVectors (
        // this makes this into an "extension" of the type. So we can call
        // trs.TransformVectors as if it was a class method.
        // Still works as a normal static function, kinda like python
		this float3x4 trs, float4x3 p, float w = 1f
	) => float4x3(
        // xx * x123 + xy * y123 + xz * z123 + xw (offset) 
		trs.c0.x * p.c0 + trs.c1.x * p.c1 + trs.c2.x * p.c2 + trs.c3.x * w,
        // yx * x123 + yy * y123 + yz * z123 + yw (offset)
		trs.c0.y * p.c0 + trs.c1.y * p.c1 + trs.c2.y * p.c2 + trs.c3.y * w,
        // zx * x123 + zy * y123 + zz * z123 + zw (offset)
		trs.c0.z * p.c0 + trs.c1.z * p.c1 + trs.c2.z * p.c2 + trs.c3.z * w
	);

    // Used for removing the unchanged values (0,0,0,1) in the last row 
    public static float3x4 TruncateTo3x4(this float4x4 m) => float3x4(m.c0.xyz, m.c1.xyz, m.c2.xyz, m.c3.xyz);
}