using Unity.Mathematics;

using static Unity.Mathematics.math;

public static partial class NoiseGen {

    /// <summary>
    /// Wrapper that applies Turbulence post process to gradient evaluation
    /// </summary>
    public struct AbsoluteTurbulent<G> : IGradientEval where G : struct,IGradientEval
    {
        public float4 Evaluate(SmallXXHashVectorized hash, float4 x)
        {
            return default(G).Evaluate(hash, x);
        }

        public float4 Evaluate(SmallXXHashVectorized hash, float4 x, float4 y)
        {
            return default(G).Evaluate(hash, x,y);
        }

        public float4 Evaluate(SmallXXHashVectorized hash, float4 x, float4 y, float4 z)
        {
            return default(G).Evaluate(hash, x,y,z);
        }

        public float4 EvaluateAfterInterpolation(float4 value)
        {
           return abs(default(G).EvaluateAfterInterpolation(value));
        }
    }
}