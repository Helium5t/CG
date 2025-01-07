
using Unity.Collections;
using Unity.Jobs;
using Unity.Mathematics;
using UnityEngine;

using static NoiseGen;

class Noise : Visualizer
{

    public static ScheduleDelegate[] noiseGenerators = {
        NoiseGenJob<LatticeNoise1D>.ScheduleParallel,
        NoiseGenJob<LatticeNoise2D>.ScheduleParallel,
        NoiseGenJob<LatticeNoise3D>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<PerlinGradient>>.ScheduleParallel,
    };

    static int noiseId = Shader.PropertyToID("_Noises");

    [SerializeField]
    int seed;

    [SerializeField]
    SpaceTRS domain = new SpaceTRS{
        scale = 8f
    };


    enum NoiseType{
        Lattice1D,
        Lattice2D,
        Lattice3D,
        Gradient1D,
        Gradient2D,
        Gradient3D,
        Perlin1D,
        Perlin2D,
        Perlin3D,
    }

    [SerializeField]
    NoiseType noiseType  = NoiseType.Lattice1D;

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
        gBufferNoise.Release();
        noises.Dispose();
        gBufferNoise = null;
    }


    protected override void UpdateViz(JobHandle shapeGenJob)
    {
        shapeGenJob.Complete();

        // string log = "10 coords:";
        // for (int i=0 ; i < 10 && i < coords.Length; i++){
        //      log += "-" + coords[i].c0.ToString();
        // }
        // Debug.Log(log);

        // string log = "domain:";
        // log += domain.Matrix.ToString();
        // Debug.Log(log);

        noiseGenerators[(int)noiseType](
            coords,
            noises,
            seed,
            resolution,
            domain, 
            shapeGenJob 
        ).Complete();
       
        // Debug.Log("Setting noise!");
        // log = "10 noises:";
        // for (int i=0 ; i < 10 && i < noises.Length; i++){
        gBufferNoise.SetData(noises.Reinterpret<float>(4 * 4));
    }
    // This delegate allows us to call a different schedule based on different noise generation we want. 
    // For each noise type, we will have a ScheduleDelegate(...) => ScheduleParallel<NoiseType>(...)
    public delegate JobHandle ScheduleDelegate(
       NativeArray<float3x4> coords, NativeArray<float4> noise,int seed, int resolution,SpaceTRS transform, JobHandle dependency
    );
}