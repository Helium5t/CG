using Unity.Mathematics;

using static Unity.Mathematics.math;

public static partial class NoiseGen {

	public interface IGradientEval {

        
        /// <summary>
        /// Generate value from hash and gradient over dimension in the range [-1,1]
        /// </summary>
		float4 Evaluate (SmallXXHashVectorized hash, float4 x);

        /// <summary>
        /// Generate value from hash and gradient over dimension in the range [-1,1]
        /// </summary>
		float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y);

        /// <summary>
        /// Generate value from hash and gradient over dimension in the range [-1,1]
        /// </summary>
		float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y, float4 z);
    }

    /// <summary>
    /// Evaluates gradient as simply the fractional part of the hash mapped to [-1,1]
    /// </summary>
    public struct FractionalGradient: IGradientEval{
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x )  => hash.MapATo01 * 2f -1f;
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y)  => hash.MapATo01 * 2f -1f;
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y, float4 z)  => hash.MapATo01 * 2f -1f;
    }

    /// <summary>
    /// Generated a value follwing Perlin noise from Hash and point gradient.
    /// </summary>
    public struct PerlinGradient: IGradientEval{
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x )  =>	(1f + hash.MapATo01) * select(-x, x, ((uint4)hash & 1 << 8/*avoids dependency on same byte */) == 0); // [0,2]
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y) {
            float4 gx = hash.MapATo01 * 2f - 1f;
			float4 gy = 0.5f - abs(gx); // Extract y coordinate from single-dimension hash
			gx -= floor(gx + 0.5f); // Extract x cooridnate from single dimension hash
			return (gx * x + gy * y) * 2f; // generates points along a rotated square of side 1 and vertices in 0.5 from the hash, then uses x and y to scale the point along it.
        }
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y, float4 z)  => 0f;
    }

    

	public struct GradientNoise1D<G> : INoiseGenerator where G: struct, IGradientEval {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash) {
			LatticeValuesVectorized x = GenerateLatticeValues(coords.c0);// get all x (lattice is 1D)

			var g = default(G); // Allow for different gradients
			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			float4 finalPoint = lerp(
				g.Evaluate(hash.Eat(x.p0), x.g0),
				g.Evaluate(hash.Eat(x.p1), x.g1),
				x.t);

			return finalPoint;
		}
	}

	public struct GradientNoise2D<G> : INoiseGenerator where G: struct, IGradientEval {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash) {
			LatticeValuesVectorized x = GenerateLatticeValues(coords.c0),// get all x coordinates
									z = GenerateLatticeValues(coords.c2);// get all z coordinates
            var g = default(G);

			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			SmallXXHashVectorized hashX0 = hash.Eat(x.p0), hashX1 = hash.Eat(x.p1);  // Start off with hashing the X Space
			float4 finalPoint = 
			lerp(
			lerp(
                g.Evaluate(hashX0.Eat(z.p0),x.g0, z.g0), 
                g.Evaluate(hashX0.Eat(z.p1),x.g0, z.g1), // hashxA.hash(zB) => (..., gradA,gradB)
                z.t),  // Hash Z space from X0 
			lerp(
                g.Evaluate(hashX1.Eat(z.p0),x.g1, z.g0), 
                g.Evaluate(hashX1.Eat(z.p1),x.g1, z.g1), 
                z.t),  // Hash Z space from X1
			x.t);  // Lerp based on X

			// Ultimately double the value and reduce by 1 => [-1.0,1.0]
			return finalPoint;
		}
	}

	public struct GradientNoise3D<G> : INoiseGenerator where G: struct, IGradientEval {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash) {
			LatticeValuesVectorized x = GenerateLatticeValues(coords.c0),// get all x coordinates
									y = GenerateLatticeValues(coords.c1),// get all y coordinates
									z = GenerateLatticeValues(coords.c2);// get all z coordinates

            var g = default(G);
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

			float4 lerpX0Y0 = lerp(
                    g.Evaluate(hashX0Y0Z0, x.g0, y.g0, z.g0),
                    g.Evaluate(hashX0Y0Z1, x.g0, y.g0, z.g1),
                    z.t),
				   lerpX0Y1 = lerp(
                    g.Evaluate(hashX0Y1Z0,x.g0,y.g1, z.g0), 
                    g.Evaluate(hashX0Y1Z1,x.g0,y.g1, z.g1),
                    z.t),
				   lerpX1Y0 = lerp(
                    g.Evaluate(hashX1Y0Z0,x.g1,y.g0,z.g0),
                    g.Evaluate(hashX1Y0Z1,x.g1,y.g0,z.g1),
                    z.t),
				   lerpX1Y1 = lerp(
                    g.Evaluate(hashX1Y1Z0,x.g1,y.g1,z.g0),
                    g.Evaluate(hashX1Y1Z1,x.g1,y.g1,z.g1),
                    z.t);
			
			float4 lerpX0 = lerp(lerpX0Y0, lerpX0Y1, y.t),
				   lerpX1 = lerp(lerpX1Y0, lerpX1Y1, y.t);
				
			float4 finalPoint = lerp(lerpX0, lerpX1, x.t);

			return finalPoint;
		}
	}
}