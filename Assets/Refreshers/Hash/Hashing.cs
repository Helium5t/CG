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

public class Hashing: MonoBehaviour
{
    [SerializeField]
    Material mat;
    [SerializeField]
    Mesh mesh;
    [SerializeField, Range(1,512)] 
    int resolution;

    ComputeBuffer gBufferHashes, gBufferCoords, gBufferNormals;

    /// <summary>
    /// coordinates and normals for the parametric shapes
    /// </summary>
    NativeArray<float3x4> coords, normals; 
    NativeArray<uint4> hashes;
    MaterialPropertyBlock mpb;

    JobHandle jobs;

    static int hashesID = Shader.PropertyToID("_Hashes"),
    resInfoID = Shader.PropertyToID("_ResolutionInfo"),
    coordsID = Shader.PropertyToID("_Coords"),
    normsID = Shader.PropertyToID("_Normals"),
    smoothID = Shader.PropertyToID("_Smoothness");

    [SerializeField]

    int seed =0;

    // [SerializeField, Range(-2f, 10f)]
	// float verticalRange = 1f;

    [SerializeField, Range(-0.5f, 0.5f)]
	float displacement = 0.1f;

    bool isDirty;
    

    // transform that is applied to all the points in the same way, regardless of coordinates. 
    // this way we can rotate, move and scale the source points of the hashing
    [SerializeField]
	SpaceTRS domain = new SpaceTRS {
		scale = 8f
	};
	
    public enum Shape { Plane, Sphere, OctaSphereGen, Torus }

	static ShapeGen.ScheduleDelegate[] shapeJobs = {
		ShapeGen.Job<ShapeGen.PlaneGen>.ScheduleParallel,
		ShapeGen.Job<ShapeGen.SphereGen>.ScheduleParallel,
		ShapeGen.Job<ShapeGen.OctaSphereGen>.ScheduleParallel,
		ShapeGen.Job<ShapeGen.TorusGen>.ScheduleParallel
	};
	
	[SerializeField]
	Shape shape;

    [SerializeField, Range(0.1f, 10f)]
	float instanceScale = 2f;

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

    public void OnEnable(){
        isDirty = true;
        int length = (resolution * resolution/4) + (resolution & 1);
        /*
        We neeed compute buffers to also increase in size in steps of 4 because size between arrays needs to match. 
        If we use base resolution as reference this is what happens:
        sizeof(gbuffer) for resolution:3 = 3 * 3 * sizeof(uint32) = 3 * 3 * 4 = 36
        sizeof(nativeArray) for resolution:3 = 3 * sizeof(uint4) = 3 * 4 * sizeof(uint) = 3 * 4 * 4 = 48
        */
        int computeBufferLength = length*4;

        hashes = new NativeArray<uint4>(length,Allocator.Persistent);
        coords = new NativeArray<float3x4>(length,Allocator.Persistent);
        normals = new NativeArray<float3x4>(length, Allocator.Persistent);
        // 4 = uint 32 bits / 8 (stride is in bytes)
        gBufferHashes = new ComputeBuffer(computeBufferLength, 4);
        // 3 (xyz) * 4 (float stride)
        gBufferCoords = new ComputeBuffer(computeBufferLength, 3 * 4);
        gBufferNormals = new ComputeBuffer(computeBufferLength, 3 * 4);

        mpb = new MaterialPropertyBlock();
        mpb.SetBuffer(hashesID, gBufferHashes);
        mpb.SetBuffer(coordsID, gBufferCoords);
        mpb.SetBuffer(normsID, gBufferNormals);
        mpb.SetVector(resInfoID, new Vector4(resolution, instanceScale / resolution, displacement/ resolution, 0f));
        mpb.SetFloat(smoothID, 1f);
    }

    public void OnDisable(){
        gBufferHashes.Release();
        gBufferCoords.Release();
        gBufferNormals.Release();
        hashes.Dispose();
        coords.Dispose();
        normals.Dispose();
        mpb = null;
        gBufferHashes = null;
        gBufferCoords = null;
        gBufferNormals = null;
    }

    public void OnValidate(){
        if (!enabled || gBufferHashes == null){
            return;
        }
        OnDisable();
        OnEnable();
    }

    Bounds bounds;

    public void Update(){
        if (isDirty || transform.hasChanged){
            bounds = new Bounds(  
				transform.position,
				float3(2f * cmax(abs(transform.lossyScale)) + displacement)
            );
            isDirty = false;
            transform.hasChanged = false;
            shapeJobs[(int) shape](coords, normals, transform.localToWorldMatrix, resolution, default).Complete();

            jobs = new HashGen(){
                hashes = hashes,
                // "pack" 4 coordinates into one matrix to make the hash computation parallel.s
                coords = coords,
                hash = SmallXXHashVectorized.Seed(seed),
                domainTransform = domain.Matrix,
            }.ScheduleParallel(hashes.Length,resolution, default); // each batch will do one row

            jobs.Complete();
            // uint32 = 4 bytes, uint4 = 4 * sizeof(uint32)
            // passed size is CURRENT expected size of the data type, not the one we are converting to.
            gBufferHashes.SetData(hashes.Reinterpret<uint>(4*4));
            // float32 = 4 bytes, float3 = 3 * sizeof(uint32), float3x4 = 4 * sizeof(float3)
            gBufferCoords.SetData(coords.Reinterpret<float3>(3 * 4 * 4));
            gBufferNormals.SetData(normals.Reinterpret<float3>(3 * 4 * 4));
        }


        RenderParams rp = new RenderParams(){
            matProps = mpb,
            material = mat,
            worldBounds = bounds,
        };
        Graphics.RenderMeshPrimitives(rp, mesh, 0, resolution * resolution);
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
