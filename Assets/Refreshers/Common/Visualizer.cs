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

using System;

public abstract class Visualizer: MonoBehaviour
{
    [SerializeField]
    protected Material mat;
    [SerializeField]
    protected Mesh mesh;
    [SerializeField, Range(1,512)] 
    protected int resolution;

    protected ComputeBuffer  gBufferCoords, gBufferNormals;

    /// <summary>
    /// coordinates and normals for the parametric shapes
    /// </summary>
    protected NativeArray<float3x4> coords, normals;
    protected MaterialPropertyBlock mpb;

    protected static int resInfoID = Shader.PropertyToID("_ResolutionInfo"),
                         coordsID = Shader.PropertyToID("_Coords"),
                         normsID = Shader.PropertyToID("_Normals"),
                         smoothID = Shader.PropertyToID("_Smoothness");

	
    public enum Shape { Plane, Sphere, OctaSphere, Torus }

	protected static ShapeGen.ScheduleDelegate[] shapeJobs = {
		ShapeGen.Job<ShapeGen.PlaneGen>.ScheduleParallel,
		ShapeGen.Job<ShapeGen.SphereGen>.ScheduleParallel,
		ShapeGen.Job<ShapeGen.OctaSphereGen>.ScheduleParallel,
		ShapeGen.Job<ShapeGen.TorusGen>.ScheduleParallel
	};
	
	[SerializeField]
	protected Shape shape;

    [SerializeField, Range(0.1f, 10f)]
	protected float instanceScale = 2f;

    [SerializeField, Range(0.1f, 1000f)]
	protected float displacement = 2f;

    // This shouldn't get modifed by implementations
    bool isDirty;

    /* 
    The tutorial here passes other parameters such as material property block, resolution etc...
    We don't actually need to pass them to the functions, we can turn them protected and have direct access.
    */ 
    protected abstract void EnableViz(int jobBufferLength );
    protected abstract void DisableViz();

    protected abstract void UpdateViz(JobHandle shapeGenJob);

    public void OnEnable(){
        int length = (resolution * resolution/4) + (resolution & 1);
        /*
        We neeed compute buffers to also increase in size in steps of 4 because size between arrays needs to match. 
        If we use base resolution as reference this is what happens:
        sizeof(gbuffer) for resolution:3 = 3 * 3 * sizeof(uint32) = 3 * 3 * 4 = 36
        sizeof(nativeArray) for resolution:3 = 3 * sizeof(uint4) = 3 * 4 * sizeof(uint) = 3 * 4 * 4 = 48
        */
        int computeBufferLength = length*4;

        coords = new NativeArray<float3x4>(length,Allocator.Persistent);
        normals = new NativeArray<float3x4>(length, Allocator.Persistent);
        // 3 (xyz) * 4 (float stride)
        gBufferCoords = new ComputeBuffer(computeBufferLength, 3 * 4);
        gBufferNormals = new ComputeBuffer(computeBufferLength, 3 * 4);

        mpb = new MaterialPropertyBlock();
        mpb.SetBuffer(coordsID, gBufferCoords);
        mpb.SetBuffer(normsID, gBufferNormals);
        mpb.SetVector(resInfoID, new Vector4(resolution, instanceScale / resolution, displacement/ resolution, 0f));
        mpb.SetFloat(smoothID, 1f);
        EnableViz(length);
    }

    public void OnDisable(){
        gBufferCoords.Release();
        gBufferNormals.Release();
        coords.Dispose();
        normals.Dispose();
        mpb = null;
        gBufferCoords = null;
        gBufferNormals = null;
        DisableViz();
    }

    public void OnValidate(){
        Debug.Log("OnValidate");
        if (!enabled || gBufferCoords == null){
            Debug.Log("Skipping either disabled or coords are null");
            return;
        }
        OnDisable();
        OnEnable();
        isDirty = true;
    }

    protected Bounds bounds;

    public void Update(){
        if (isDirty){
            Debug.Log("isDirty");
        }
        if (transform.hasChanged){
            Debug.Log("transformChanged");
        }
        if (isDirty || transform.hasChanged){
            Debug.Log("UpdateViz");
            bounds = new Bounds(  
				transform.position,
				float3(2f * cmax(abs(transform.lossyScale)) + displacement)
            );
            UpdateViz(shapeJobs[(int) shape](coords, normals, transform.localToWorldMatrix, resolution, default));
            // float32 = 4 bytes, float3 = 3 * sizeof(uint32), float3x4 = 4 * sizeof(float3)
            gBufferCoords.SetData(coords.Reinterpret<float3>(3 * 4 * 4));
            gBufferNormals.SetData(normals.Reinterpret<float3>(3 * 4 * 4));
            if (isDirty){
                isDirty = false;
                // WaitForSeconds wfs = new WaitForSeconds(0.3f);
                // while(wfs.)
            }
            transform.hasChanged = false;
        }
        RenderParams rp = new RenderParams(){
            matProps = mpb,
            material = mat,
            worldBounds = bounds,
        };
        Graphics.RenderMeshPrimitives(rp, mesh, 0, resolution * resolution);
    }
}