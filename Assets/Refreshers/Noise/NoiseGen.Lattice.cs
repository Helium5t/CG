using Unity.Mathematics;

using static Unity.Mathematics.math;
using static Noise;
using System.Security.Cryptography.X509Certificates;
using UnityEngine.InputSystem.Android;

public static partial class NoiseGen {

	struct LatticeValuesVectorized{
		public int4 p0,p1;
		public float4 t;
	}

	static LatticeValuesVectorized GenerateLatticeValues(float4 coordinates){
		
		// get integer part
		float4 points = floor(coordinates);
		return new LatticeValuesVectorized{
			p0 = (int4) points,
			p1 = (int4) points + 1,
			t = coordinates - points,
		};
	}

	public struct LatticeNoise1D : INoiseGenerator {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash) {
			LatticeValuesVectorized x = GenerateLatticeValues(coords.c0);// get all x (lattice is 1D)

			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			float4 finalPoint = lerp(hash.Eat(x.p0).MapATo01, hash.Eat(x.p1).MapATo01 , coords.c0 - x.t);

			// Ultimately double the value and reduce by 1 => [-1.0,1.0]
			return (finalPoint *2f) - 1f;
		}
	}

	public struct LatticeNoise2D : INoiseGenerator{

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash) {
			LatticeValuesVectorized x = GenerateLatticeValues(coords.c0),// get all x coordinates
									z = GenerateLatticeValues(coords.c2);// get all z coordinates

			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			SmallXXHashVectorized hashX0 = hash.Eat(x.p0), hashX1 = hash.Eat(x.p1);  // Start off with hashing the X Space
			float4 finalPoint = 
			lerp(
			lerp(hashX0.Eat(z.p0).MapATo01, hashX0.Eat(z.p1).MapATo01 , z.t),  // Hash Z space from X0 
			lerp(hashX1.Eat(z.p0).MapATo01, hashX1.Eat(z.p1).MapATo01 , z.t),  // Hash Z space from X1
			x.t);  // Lerp based on X

			// Ultimately double the value and reduce by 1 => [-1.0,1.0]
			return finalPoint *2f - 1f;
		}
	}
}