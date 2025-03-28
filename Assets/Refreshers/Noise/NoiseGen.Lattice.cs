using Unity.Mathematics;

using static Unity.Mathematics.math;
using static Noise;
using System.Security.Cryptography.X509Certificates;
using UnityEngine.InputSystem.Android;
using UnityEngine;

public static partial class NoiseGen {

	// Requirements of a smoothing function is that f(x) is in [0,1] for each x in [0,1].
	// Also f(0) = 0 and f(1) = 1
	// f(t) = 6t^5 - 15t^4 + 10t^3 => f(0) = 0 , f(1) = 6-15+10 = 1 
	// A function that has solution in 0 and 1 in both its first and second derivative (C2-Continuous)
	// f' (t) = 30t4 - 60t3 + 30t2 
	// f''(t) = 120t3 - 180t2 + 60t
	// The c2 continuity ensures that there wont' be major changes in value at the thresholds between one value and the next in the noise
	private static float4 customSmoothing(float4 t) => t * t * t * (t * (t * 6f - 15f) + 10f);
	
	public struct StructureValuesVectorized{
		public int4 p0, p1;
		public float4 g0, g1; // To represent gradient with respect to each point.
		public float4 t;
	}


	public struct LatticeNoise1D<T> : INoiseGenerator where T: struct, INoiseStructure{

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			StructureValuesVectorized x = default(T).GenerateNoiseStructure(coords.c0, frequency);// get all x coordinates
								

			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			SmallXXHashVectorized hashX0 = hash.Eat(x.p0), hashX1 = hash.Eat(x.p1);  // Start off with hashing the X Space
			float4 finalPoint = lerp(hashX0.MapATo01,hashX1.MapATo01,x.t);

			// Ultimately double the value and reduce by 1 => [-1.0,1.0]
			return finalPoint *2f - 1f;
		}
	}

	public struct LatticeNoise2D<T> : INoiseGenerator where T: struct, INoiseStructure{

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			StructureValuesVectorized x = default(T).GenerateNoiseStructure(coords.c0,frequency),// get all x coordinates
									z = default(T).GenerateNoiseStructure(coords.c2, frequency);// get all z coordinates

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
	public struct LatticeNoise3D<T> : INoiseGenerator where T: struct, INoiseStructure{

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			StructureValuesVectorized x = default(T).GenerateNoiseStructure(coords.c0, frequency),// get all x coordinates
									y = default(T).GenerateNoiseStructure(coords.c1, frequency),// get all y coordinates
									z = default(T).GenerateNoiseStructure(coords.c2, frequency);// get all z coordinates

			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			SmallXXHashVectorized hashX0 = hash.Eat(x.p0), hashX1 = hash.Eat(x.p1);  // Start off with hashing the X Space
			SmallXXHashVectorized hashX0Y0 = hashX0.Eat(y.p0),
								  hashX0Y1 = hashX0.Eat(y.p1),
								  hashX1Y0 = hashX1.Eat(y.p0),
								  hashX1Y1 = hashX1.Eat(y.p1);


			SmallXXHashVectorized hashX0Y0Z0 = hashX0Y0.Eat(z.p0),
								  hashX0Y0Z1 = hashX0Y0.Eat(z.p1),
								  hashX0Y1Z0 = hashX0Y1.Eat(z.p0),
								  hashX0Y1Z1 = hashX0Y1.Eat(z.p1),
								  hashX1Y0Z0 = hashX1Y0.Eat(z.p0),
								  hashX1Y0Z1 = hashX1Y0.Eat(z.p1),
								  hashX1Y1Z0 = hashX1Y1.Eat(z.p0),
								  hashX1Y1Z1 = hashX1Y1.Eat(z.p1);

			float4 lerpX0Y0 = lerp(hashX0Y0Z0.MapATo01, hashX0Y0Z1.MapATo01, z.t),
				   lerpX0Y1 = lerp(hashX0Y1Z0.MapATo01, hashX0Y1Z1.MapATo01, z.t),
				   lerpX1Y0 = lerp(hashX1Y0Z0.MapATo01, hashX1Y0Z1.MapATo01, z.t),
				   lerpX1Y1 = lerp(hashX1Y1Z0.MapATo01, hashX1Y1Z1.MapATo01, z.t);
			
			float4 lerpX0 = lerp(lerpX0Y0, lerpX0Y1, y.t),
				   lerpX1 = lerp(lerpX1Y0, lerpX1Y1, y.t);
				
			float4 finalPoint = lerp(lerpX0, lerpX1, x.t);

			return finalPoint *2f - 1f;
		}
	}
}