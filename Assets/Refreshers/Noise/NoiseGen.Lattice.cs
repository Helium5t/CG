using Unity.Mathematics;

using static Unity.Mathematics.math;
using static Noise;

public static partial class NoiseGen {

	public struct LatticeNoise1D : INoiseGenerator {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash) {
			// Map coordinates to [-1,1] space
			// get integer part
			int4 p = (int4)floor(coords.c0);// get all x (lattice is 1D)
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			float4 v = (uint4)hash.Eat(p) & 255;
			// divide by half of value range [0.0, 255.0] => [0,2]
			// and reduce by 1 => [-1,1]
			return v * (2f / 255f) - 1f;
		}
	}
}