
using Unity.Collections;
using Unity.Jobs;
using Unity.Mathematics;
using UnityEngine;
using Unity.Jobs;

using static NoiseGen;

class Noise : Visualizer
{

    static int noiseId = Shader.PropertyToID("_Noise");

    [SerializeField]
    int seed;

    [SerializeField]
    SpaceTRS domain = new SpaceTRS{
        scale = 8f
    };

    NativeArray<float4> noises;

    ComputeBuffer gBufferNoise;
    protected override void EnableViz(int jobBufferLength)
    {
        noises = new NativeArray<float4>(jobBufferLength,Allocator.Persistent);
        // Stride is 4 because the buffer will read floats individually instead of in packed form.
        gBufferNoise = new ComputeBuffer(jobBufferLength * 4, 4);
        mpb.SetBuffer(noiseId, gBufferNoise);
    }
    protected override void DisableViz()
    {
        noises.Dispose();
        gBufferNoise.Release();
        gBufferNoise = null;
    }


    protected override void UpdateViz(JobHandle shapeGenJob)
    {
        NoiseGenJob<LatticeNoise1D>.ScheduleParallel(coords, noises, seed, resolution,domain, shapeGenJob ).Complete();
        
        gBufferNoise.SetData(noises.Reinterpret<float>(4 * 4));
    }
}