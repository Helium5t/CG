using System.Net.NetworkInformation;
using Palmmedia.ReportGenerator.Core;
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

        GenSettings genSettings;

		public float3x4 transform;

		public void Execute (int i) {
            float4x3 transformedCoords  = transform.TransformVectors(transpose(coords[i]));
            int f = genSettings.frequency ;
            SmallXXHashVectorized hashes = SmallXXHashVectorized.Seed(genSettings.seed);

            float4 finalNoise = 0f;
            float amplitude = 1f, maxAmplitude = 0f;
            for (int o =0; o < genSettings.octaves; o++){
                finalNoise =+ amplitude * default(N).GenerateVectorizedNoise(
                    // Scale the coordinate up by the frequency to increase rate of change across same value space
                    f* transformedCoords, hashes
                );
                // Increase frequency
                f*=2;
                maxAmplitude += amplitude;
                // decrease amplitude
                amplitude *= 0.5f;
            }
            // Divide by max amplitude to normalize the noise in [-1,1]
			noise[i] = finalNoise/ maxAmplitude;
		}

        public static JobHandle ScheduleParallel(NativeArray<float3x4> coords, NativeArray<float4> noise,GenSettings settings, int resolution,SpaceTRS transform, JobHandle dependency ) => 
        new NoiseGenJob<N>{
                coords = coords,
                noise = noise,
                genSettings = settings,
                transform = transform.Matrix
            }.ScheduleParallel(coords.Length,resolution, dependency);
        
    }
}