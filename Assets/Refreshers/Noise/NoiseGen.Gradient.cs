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

        /// <summary>
        /// "Post-processing" function after interpolation between lattice points is done.
        /// </summary>
        float4 EvaluateFinal(float4 value);    
    }

    public static class VectorGenerator {
        public static float4 GenerateLineGradient(SmallXXHashVectorized hash, float4 x) =>
            	(1f + hash.MapATo01) * select(-x, x, ((uint4)hash & 1 << 8/*avoids dependency on same byte */) == 0); // [0,2]
            
        /// <summary>
        /// Generate values along a square given a one dimensional hash.
        /// </summary>
        static float4x2 GenerateSquareCoordinates(SmallXXHashVectorized hash){
            float4x2 v;
            v.c0 = hash.MapATo01 * 2f -1f; //[-1,1]
            v.c1 = 0.5f - abs(v.c0); // [-0.5, 0.5]
            v.c0 -= floor(v.c0 + 0.5f); // v.c0 - [-1,0,1] => fractional part of value.
            return v;
        }

        /// <summary>
        /// Generate values along a octahedron given a one dimensional hash.
        /// </summary>
        static float4x3 GenerateOctahedronCoordinates( SmallXXHashVectorized h) {
            float4x3 g;
            g.c0 = h.MapATo01 * 2f - 1f;
            g.c1 = h.MapBTo01 * 2f - 1f;
            g.c2 = 1f - abs(g.c0) - abs(g.c1); // Same approach as square to "fold" a onedmensional line
            float4 offset = max(-g.c2, 0f); 
            g.c0 +=  select(-offset, offset, g.c0 < 0f); // Fold again beyond the midpoint;
            g.c1 +=  select(-offset, offset, g.c1 < 0f);
            return g;
        }

        public static float4 GenerateSquareGradient( SmallXXHashVectorized h, float4 x, float4 y){
            float4x2 v = GenerateSquareCoordinates(h);
            return v.c0 * x + v.c1 * y;
        }

       public static float4 GenerateCircleGradient( SmallXXHashVectorized h, float4 x, float4 y){
            float4x2 v = GenerateSquareCoordinates(h);
            // Rsqrt normalizes the vector to get a circle from a square
            return (v.c0 * x + v.c1 * y) * rsqrt(v.c0 * v.c0 + v.c1 * v.c1); 
		}

        public static float4 GenerateOctahedronGradient(SmallXXHashVectorized h, float4 x, float4 y, float4 z){
            float4x3 v = GenerateOctahedronCoordinates(h);
            return v.c0 * x + v.c1 * y + v.c2 * z;
        }
        public static float4 GenerateSphereGradient(SmallXXHashVectorized h, float4 x, float4 y, float4 z){
            float4x3 v = GenerateOctahedronCoordinates(h);
            // Rsqrt normalizes the vector to get a sphere from a octahedron
            return (v.c0 * x + v.c1 * y + v.c2 * z) *rsqrt(v.c0 * v.c0 + v.c1 * v.c1 + v.c2 * v.c2);
        }
    }

    /// <summary>
    /// Evaluates gradient as simply the fractional part of the hash mapped to [-1,1]
    /// </summary>
    public struct FractionalGradient: IGradientEval{
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x )  => hash.MapATo01 * 2f -1f;
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y)  => hash.MapATo01 * 2f -1f;
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y, float4 z)  => hash.MapATo01 * 2f -1f;
        public float4 EvaluateFinal(float4 value){
            return value;
        }
    }

    /// <summary>
    /// Generated a value follwing Perlin noise from Hash and point gradient.
    /// </summary>
    public struct PerlinGradient: IGradientEval{
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x )  =>	VectorGenerator.GenerateLineGradient(hash, x);
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y) {
            return VectorGenerator.GenerateSquareGradient(hash, x, y) * (2f / 0.53528f);
            /*
            OLD VERSION THAT DID NOT USE SHAPE GENERATOR
            */
            float4 gx = hash.MapATo01 * 2f - 1f;
			float4 gy = 0.5f - abs(gx); // Extract y coordinate from single-dimension hash
			gx -= floor(gx + 0.5f); // Extract x cooridnate from single dimension hash
            /* Given a quirk of the custom smoothing,
            with some gradient configuration we can reach a max value of 0.53528..,
            so we divide by that to normalize */
			return (gx * x + gy * y) * (2f / 0.53528f); // generates points along a rotated square of side 1 and vertices in 0.5 from the hash, then uses x and y to scale the point along it.
        }


		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y, float4 z) {
            return VectorGenerator.GenerateOctahedronGradient(hash, x,y,z) * (1f / 0.5629f);
            /*
            OLD VERSION THAT DID NOT USE SHAPE GENERATOR
            */
            // 3D version of 2D square generation, equivalent to octahedron sphere generation in ShapeGen.cs
            float4  gx = hash.MapATo01 * 2f - 1f, 
                    gy = hash.MapDTo01 * 2f - 1f; // Use byte 0 and 3 of the hash as inputs
			float4 gz = 1f - abs(gx) - abs(gy); 
			float4 offset = max(-gz, 0f);
			gx += select(-offset, offset, gx < 0f);
			gy += select(-offset, offset, gy < 0f);
			return (gx * x + gy * y + gz * z) * (1f / 0.56290f)/*Normalization based on max values of function*/;
        }


         public float4 EvaluateFinal(float4 value){
            return value;
        }
    }

    
    public struct SimplexGradient : IGradientEval{
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x)=>
            VectorGenerator.GenerateLineGradient(hash, x)  * (32f / 27f); // scale to normalize.

        public float4 Evaluate(SmallXXHashVectorized hash, float4 x, float4 y)=>
            VectorGenerator.GenerateCircleGradient(hash, x, y)* (5.832f / sqrt(2f));
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x, float4 y, float4 z)=>
            VectorGenerator.GenerateSphereGradient(hash, x,y,z)* (1024f / (125f * sqrt(3f)));

        public float4 EvaluateFinal(float4 value) => value;
    }

	public struct GradientNoise1D<T,G> : INoiseGenerator where G: struct, IGradientEval where T: struct, INoiseStructure {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			StructureValuesVectorized x = default(T).GenerateNoiseStructure(coords.c0, frequency);// get all x (lattice is 1D)

			var g = default(G); // Allow for different gradients
			// Map coordinates to [-1,1] space
			// hash the values, mask the first byte and then convert to float. [0.0,255.0]
			// divide by value range [0.0, 255.0] => [0,0,1.0]
			// interpolate between actual point and next value based on the fractional part of the x coordinate.
			float4 finalPoint = lerp(
				g.Evaluate(hash.Eat(x.p0), x.g0),
				g.Evaluate(hash.Eat(x.p1), x.g1),
				x.t);

			return g.EvaluateFinal(finalPoint);
		}
	}

	public struct GradientNoise2D<T,G> : INoiseGenerator where G: struct, IGradientEval where T: struct, INoiseStructure {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			StructureValuesVectorized x = default(T).GenerateNoiseStructure(coords.c0, frequency),// get all x coordinates
									z = default(T).GenerateNoiseStructure(coords.c2, frequency);// get all z coordinates
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
			return g.EvaluateFinal(finalPoint);
		}
	}

	public struct GradientNoise3D<T,G> : INoiseGenerator where G: struct, IGradientEval where T: struct, INoiseStructure {

		public float4 GenerateVectorizedNoise(float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			StructureValuesVectorized x = default(T).GenerateNoiseStructure(coords.c0, frequency),// get all x coordinates
									y = default(T).GenerateNoiseStructure(coords.c1, frequency),// get all y coordinates
									z = default(T).GenerateNoiseStructure(coords.c2, frequency);// get all z coordinates

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

			return g.EvaluateFinal(finalPoint);
		}
	}
}

