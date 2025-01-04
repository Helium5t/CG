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
        

        private float4x3 TransformPositionVectorized(float3x4 transform, float4x3 positions) =>  float4x3(
            // xx * x123 + xy * y123 + xz * z123 + xw (offset) 
			transform.c0.x * positions.c0 + transform.c1.x * positions.c1 + transform.c2.x * positions.c2 + transform.c3.x,
            // yx * x123 + yy * y123 + yz * z123 + yw (offset)
			transform.c0.y * positions.c0 + transform.c1.y * positions.c1 + transform.c2.y * positions.c2 + transform.c3.y,
            // zx * x123 + zy * y123 + zz * z123 + zw (offset)
			transform.c0.z * positions.c0 + transform.c1.z * positions.c1 + transform.c2.z * positions.c2 + transform.c3.z
		);

		public void Execute(int i) {
            /* previous planar point generation , now positions are offered by the previous burst code
            // Each point will generate based on coordinates and not index in the array, should generate a different noise
			float vf = floor(invResolution * i + 0.00001f); // row = index // rowSize  = 1,2,3,4
			float uf = invResolution * (i - resolution * vf + 0.5f) - 0.5f; // column = ((index % rowSize) + 0.5) / rowSize ) - 0.5 = -0.5,....., 0.5 => [-0.5,0.5]
			vf = invResolution * (vf + 0.5f) - 0.5f; // [-0.5,-0.5]
            */
            float4x3 p = TransformPositionVectorized(domainTransform,  transpose(coords[i]));

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

// https://xxhash.com/ skipping some steps (2,3,4 from https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md)
public readonly struct SmallXXHash {

	const uint primeA = 0b10011110001101110111100110110001;
	const uint primeB = 0b10000101111010111100101001110111;
	const uint primeC = 0b11000010101100101010111000111101;
	const uint primeD = 0b00100111110101001110101100101111;
	const uint primeE = 0b00010110010101100110011110110001;

    readonly uint accumulator;

	public SmallXXHash (uint seed) {
		this.accumulator = seed + primeE;
	}
    public static implicit operator SmallXXHash (uint accumulator) =>
		new SmallXXHash(accumulator);

    /// <summary>
    /// implements final mixing steps of the accumulator
    /// </summary>
    /// <param name="hash"></param>
	public static implicit operator uint (SmallXXHash hash){
        uint avalanche = hash.accumulator;
		avalanche ^= avalanche >> 15;
		avalanche *= primeB;
		avalanche ^= avalanche >> 13;
		avalanche *= primeC;
		avalanche ^= avalanche >> 16;
		return avalanche;
    }

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits
    /// </summary>
    /// <param name="data"></param>
   	public SmallXXHash Eat (int data) =>
		RotateLeft(accumulator + (uint)data * primeC, 17) * primeD;

	

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits, uses different primes when using bytes vs using int
    /// </summary>
    /// <param name="data"></param>
   public SmallXXHash Eat (byte data) =>
		RotateLeft(accumulator + data * primeE, 11) * primeA;

    static uint RotateLeft (uint data, int steps) =>
    (data << steps) | (data >> 32 - steps);

    public static SmallXXHash Seed (int seed) => (uint)seed + primeE;


	public static implicit operator SmallXXHashVectorized (SmallXXHash hash) =>
		new SmallXXHashVectorized(hash.accumulator);
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



// https://xxhash.com/ skipping some steps (2,3,4 from https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md)
// Reimplementation of SmallXXHash, but made so a burst job can vectorize the process.
public readonly struct SmallXXHashVectorized {

	// const uint primeA = 0b10011110001101110111100110110001;
	const uint primeB = 0b10000101111010111100101001110111;
	const uint primeC = 0b11000010101100101010111000111101;
	const uint primeD = 0b00100111110101001110101100101111;
	const uint primeE = 0b00010110010101100110011110110001;

    readonly uint4 accumulator;

	public SmallXXHashVectorized (uint4 seed) {
		this.accumulator = seed + primeE;
	}
    public static implicit operator SmallXXHashVectorized (uint4 accumulator) =>
		new SmallXXHashVectorized(accumulator);

    /// <summary>
    /// implements final mixing steps of the accumulator
    /// </summary>
    /// <param name="hash"></param>
	public static implicit operator uint4 (SmallXXHashVectorized hash){
        uint4 avalanche = hash.accumulator;
		avalanche ^= avalanche >> 15;
		avalanche *= primeB;
		avalanche ^= avalanche >> 13;
		avalanche *= primeC;
		avalanche ^= avalanche >> 16;
		return avalanche;
    }

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits
    /// </summary>
    /// <param name="data"></param>
   	public SmallXXHashVectorized Eat (int4 data) =>
		RotateLeft(accumulator + (uint4)data * primeC, 17) * primeD;

    static uint4 RotateLeft (uint4 data, int steps) =>
    (data << steps) | (data >> 32 - steps);

    public static SmallXXHashVectorized Seed (int4 seed) => (uint4)seed + primeE;
}
