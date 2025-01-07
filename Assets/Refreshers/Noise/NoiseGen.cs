using System.Net.NetworkInformation;
using Unity.Burst;
using Unity.Collections;
using Unity.Jobs;
using Unity.Mathematics;

using static Unity.Mathematics.math;


// partial allows code implementation to be distirbuted over multiple files
public static partial class NoiseGen {

	public interface INoiseGenerator {
		float4 GenerateVectorizedNoise (float4x3 positions, SmallXXHashVectorized hash);
	}

	[BurstCompile(FloatPrecision.Standard, FloatMode.Fast, CompileSynchronously = true)]
	public struct NoiseGenJob<N> : IJobFor where N : struct, INoiseGenerator {



		[ReadOnly]
		public NativeArray<float3x4> coords;

		[WriteOnly]
		public NativeArray<float4> noise;
        // hashes packed in 4 
		public SmallXXHashVectorized hashes;

		public float3x4 transform;

		public void Execute (int i) {
			noise[i] = default(N).GenerateVectorizedNoise(
			transform.TransformVectors(transpose(coords[i])), hashes);
		}

        public static JobHandle ScheduleParallel(NativeArray<float3x4> coords, NativeArray<float4> noise,int seed, int resolution,SpaceTRS transform, JobHandle dependency ) => 
        new NoiseGenJob<N>{
                coords = coords,
                noise = noise,
                hashes = SmallXXHash.Seed(seed),
                transform = transform.Matrix
            }.ScheduleParallel(coords.Length,resolution, dependency);
        
    }
}