
using System;
using Unity.Collections;
using Unity.Jobs;
using Unity.Mathematics;
using UnityEngine;

using static NoiseGen;

class Noise : Visualizer
{

    public bool tiling = false;

    public static ScheduleDelegate[] noiseGenerators = {
        NoiseGenJob<LatticeNoise1D<OpenLattice>>.ScheduleParallel,
        NoiseGenJob<LatticeNoise2D<OpenLattice>>.ScheduleParallel,
        NoiseGenJob<LatticeNoise3D<OpenLattice>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<OpenLattice, FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<OpenLattice, FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<OpenLattice, FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<OpenLattice, AbsoluteTurbulent<FractionalGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<OpenLattice, AbsoluteTurbulent<FractionalGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<OpenLattice, AbsoluteTurbulent<FractionalGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<OpenLattice, PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<OpenLattice, PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<OpenLattice, PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<OpenLattice, AbsoluteTurbulent<PerlinGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<OpenLattice, AbsoluteTurbulent<PerlinGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<OpenLattice, AbsoluteTurbulent<PerlinGradient>>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, EuclideanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, EuclideanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, EuclideanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, EuclideanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, EuclideanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, EuclideanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, EuclideanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, EuclideanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, EuclideanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, ChessboardDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, ChessboardDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, ChessboardDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, ChessboardDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, ChessboardDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, ChessboardDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, ChessboardDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, ChessboardDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, ChessboardDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, ManhattanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, ManhattanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, ManhattanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, ManhattanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, ManhattanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, ManhattanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<OpenLattice, ManhattanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<OpenLattice, ManhattanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<OpenLattice, ManhattanDistance, F2MinusF1>>.ScheduleParallel,
    };
    public static ScheduleDelegate[] tilingNoiseGenerators = {
        NoiseGenJob<LatticeNoise1D<TilingLattice>>.ScheduleParallel,
        NoiseGenJob<LatticeNoise2D<TilingLattice>>.ScheduleParallel,
        NoiseGenJob<LatticeNoise3D<TilingLattice>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<TilingLattice, FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<TilingLattice, FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<TilingLattice, FractionalGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<TilingLattice, AbsoluteTurbulent<FractionalGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<TilingLattice, AbsoluteTurbulent<FractionalGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<TilingLattice, AbsoluteTurbulent<FractionalGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<TilingLattice, PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<TilingLattice, PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<TilingLattice, PerlinGradient>>.ScheduleParallel,
        NoiseGenJob<GradientNoise1D<TilingLattice, AbsoluteTurbulent<PerlinGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise2D<TilingLattice, AbsoluteTurbulent<PerlinGradient>>>.ScheduleParallel,
        NoiseGenJob<GradientNoise3D<TilingLattice, AbsoluteTurbulent<PerlinGradient>>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, EuclideanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, EuclideanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, EuclideanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, EuclideanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, EuclideanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, EuclideanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, EuclideanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, EuclideanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, EuclideanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, ChessboardDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, ChessboardDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, ChessboardDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, ChessboardDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, ChessboardDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, ChessboardDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, ChessboardDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, ChessboardDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, ChessboardDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, ManhattanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, ManhattanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, ManhattanDistance, F1>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, ManhattanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, ManhattanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, ManhattanDistance, F2>>.ScheduleParallel,
        NoiseGenJob<Voronoi1D<TilingLattice, ManhattanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi2D<TilingLattice, ManhattanDistance, F2MinusF1>>.ScheduleParallel,
        NoiseGenJob<Voronoi3D<TilingLattice, ManhattanDistance, F2MinusF1>>.ScheduleParallel,
    };

    static int noiseId = Shader.PropertyToID("_Noises");

    [SerializeField]
    GenSettings genSettings;

    [SerializeField]
    SpaceTRS domain = new SpaceTRS{
        scale = 8f
    };

    enum NoiseType{
        Lattice,
        Gradient,
        TurbulentGradient,
        Perlin,
        TurbulentPerlin, 
        /// <summary>
        /// Use shortest distance as noise value
        /// </summary>
        VoronoiWorleyF1,   
        /// <summary>
        /// Use second shortest distance as noise value
        /// </summary>
        VoronoiWorleyF2,   
        VoronoiWorleyF2MinusF1,   
        VoronoiChebyshevF1,   
        VoronoiChebyshevF2,   
        VoronoiChebyshevF2MinusF1,   
        VoronoiManhattanF1,   
        VoronoiManhattanF2,   
        VoronoiManhattanF2MinusF1,   
    }

    [SerializeField]
    NoiseType noiseType  = NoiseType.Lattice;

    [SerializeField, Range(1,3)]
    int dimensions = 1;

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

        int noiseIdx = ((int)noiseType*3) + dimensions-1;

        // string log = "10 coords:";
        // for (int i=0 ; i < 10 && i < coords.Length; i++){
        //      log += "-" + coords[i].c0.ToString();
        // }
        // Debug.Log(log);

        // string log = "domain:";
        // log += domain.Matrix.ToString();
        // Debug.Log(log);
        if (tiling){
            tilingNoiseGenerators[noiseIdx](
                coords,
                noises,
                genSettings, 
                resolution,
                domain, 
                shapeGenJob 
            ).Complete();
        }else{
            noiseGenerators[noiseIdx](
                coords,
                noises,
                genSettings, 
                resolution,
                domain, 
                shapeGenJob 
            ).Complete();
        }
       
        // Debug.Log("Setting noise!");
        // log = "10 noises:";
        // for (int i=0 ; i < 10 && i < noises.Length; i++){
        gBufferNoise.SetData(noises.Reinterpret<float>(4 * 4));
    }
    // This delegate allows us to call a different schedule based on different noise generation we want. 
    // For each noise type, we will have a ScheduleDelegate(...) => ScheduleParallel<NoiseType>(...)
    public delegate JobHandle ScheduleDelegate(
       NativeArray<float3x4> coords, NativeArray<float4> noise,GenSettings settings, int resolution,SpaceTRS transform, JobHandle dependency
    );
}