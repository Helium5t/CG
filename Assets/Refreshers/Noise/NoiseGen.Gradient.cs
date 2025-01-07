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

    public struct GradientValue: IGradientEval{

        /// <summary>
        /// Map fractal part of hash: [0.0001,0.9999] to [-1,1]
        /// </summary>
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x )  => hash.MapATo01 * 2f -1f;

        /// <summary>
        /// Map fractal part of hash: [0.0001,0.9999] to [-1,1]
        /// </summary>
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y)  => hash.MapATo01 * 2f -1f;

        /// <summary>
        /// Map fractal part of hash: [0.0001,0.9999] to [-1,1]
        /// </summary>
		public float4 Evaluate (SmallXXHashVectorized hash, float4 x, float4 y, float4 z)  => hash.MapATo01 * 2f -1f;
    }
}