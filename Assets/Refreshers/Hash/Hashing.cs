using UnityEngine;
using Unity.Burst;
using Unity.Jobs;

using Unity.Collections;
using Unity.VisualScripting;
using Unity.Jobs.LowLevel.Unsafe;
using NUnit.Framework.Internal;


using static Unity.Mathematics.math;
using Unity.Mathematics;
using float4x4 = Unity.Mathematics.float4x4;
using quaternion = Unity.Mathematics.quaternion;

using Unity.Mathematics;
using System;

public class Hashing: Visualizer
{
    ComputeBuffer gBufferHashes;
    NativeArray<uint4> hashes;
    JobHandle jobs;

    static int hashesID = Shader.PropertyToID("_Hashes");

    [SerializeField]
    int seed =0;
    

    // transform that is applied to all the points in the same way, regardless of coordinates. 
    // this way we can rotate, move and scale the source points of the hashing
    [SerializeField]
	SpaceTRS domain = new SpaceTRS {
		scale = 8f
	};

    [BurstCompile(CompileSynchronously = true, FloatPrecision = FloatPrecision.Standard, FloatMode = FloatMode.Fast)]
    struct HashGen : IJobFor{

        [WriteOnly]
        public NativeArray<uint4> hashes;
        [ReadOnly]
        public NativeArray<float3x4>  coords;

        public SmallXXHashVectorized hash;

        public float3x4 domainTransform;

		public void Execute(int i) {
            /* previous planar point generation , now positions are offered by the previous burst code
            // Each point will generate based on coordinates and not index in the array, should generate a different noise
			float vf = floor(invResolution * i + 0.00001f); // row = index // rowSize  = 1,2,3,4
			float uf = invResolution * (i - resolution * vf + 0.5f) - 0.5f; // column = ((index % rowSize) + 0.5) / rowSize ) - 0.5 = -0.5,....., 0.5 => [-0.5,0.5]
			vf = invResolution * (vf + 0.5f) - 0.5f; // [-0.5,-0.5]
            */
            float4x3 p = domainTransform.TransformVectors(transpose(coords[i]));
            // Previously TransformPositionVectorized(domainTransform,  transpose(coords[i]));

			int4 u = (int4)floor(p.c0);
			int4 v = (int4)floor(p.c1);
            int4 w = (int4)floor(p.c2);
            // Old one using static domain coordinates
		    // int u = (int)floor(uf * 8f);
			// int v = (int)floor(vf * 8f); // 8 = 32/4

            hashes[i] = hash.Eat(u).Eat(v).Eat(w);
            // hashes[i] = (uint)(frac(u*v * 0.381f) * 256f);
        }
    }

    protected override void EnableViz(int jobBufferLength){
        hashes = new NativeArray<uint4>(jobBufferLength,Allocator.Persistent);
        // 4 = uint 32 bits / 8 (stride is in bytes)
        gBufferHashes = new ComputeBuffer(jobBufferLength * 4, 4);
        mpb.SetBuffer(hashesID, gBufferHashes);
    }

    protected override void DisableViz(){
        gBufferHashes.Release();
        hashes.Dispose();
        gBufferHashes = null;
    }
    /* 
    Not needed anymore as this is declared in ../Common/Visualizer.cs
     public void OnValidate(){} 
    */


    protected override void UpdateViz(JobHandle shapeGenJob){
        jobs = new HashGen(){
            hashes = hashes,
            coords = coords,
            hash = SmallXXHashVectorized.Seed(seed),
            domainTransform = domain.Matrix,
        }.ScheduleParallel(hashes.Length,resolution, shapeGenJob); // each batch will do one row

        jobs.Complete();
        // uint32 = 4 bytes, uint4 = 4 * sizeof(uint32)
        // passed size is CURRENT expected size of the data type, not the one we are converting to.
        gBufferHashes.SetData(hashes.Reinterpret<uint>(4*4));
    }
}


[System.Serializable]
public struct SpaceTRS {

	public float3 translation, rotation, scale;
    public float3x4 Matrix {
		get {
			return Util.TRS(
				translation, rotation, scale
			);
		}
	}
}

