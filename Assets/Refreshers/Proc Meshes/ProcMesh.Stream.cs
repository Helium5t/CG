using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.Rendering;


namespace ProcMesh{
    /// <summary>
    /// Handles stream packing 
    /// </summary>
    public interface IMeshStream{
        void Setup(Mesh.MeshData data, int numOfVertices, int numOfIndices, Bounds b);

        void SetVertexBuffer(int index, VertexInfo v);

        /// <summary>
        /// Sets the index buffer starting from index. e.g. <br \>
        /// SetTriangle(3, int3(1,2,3)) will set the index buffer like so: <br \>
        /// [X,X,X,1,2,3,...]
        /// </summary>
        void SetTriangle(int index, int3 triangleIndices);
    }

    public struct SingleStream : IMeshStream
    {
        /// <summary>
        /// A structure matching ProcMesh.VertexInfo used to describe data structure to advanced API.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        struct StreamVertex{
            public float3 position, normal;
            public float4 tangent;
            public float2 uv0;
        }

        // stream0 and indices are two native array that point to an unmanaged data block
        // The advanced API exposes the mesh data as an unmanaged block, thus Unity seed the access
        // to the two as access to the same data block. Given the risk of overlap and thus write/read 
        // faults, it would block compilation if we don't use NativeDisableContainerSafetyRestriction.
        // We are sure access does not overlap because we are effectively accessing always two separate 
        // areas of the mesh data, streams and indices.
        [NativeDisableContainerSafetyRestriction]
        NativeArray<StreamVertex> stream0;

        [NativeDisableContainerSafetyRestriction]
        NativeArray<utri16> indices;

        private static readonly int numOfVertexAttributes = 4;
        public void SetTriangle(int index, int3 triangleIndices)
        {
            indices[index] = triangleIndices;
        }

        public void Setup(Mesh.MeshData data, int numOfVertices, int numOfIndices, Bounds b)
        {
            data.subMeshCount = 1;
            NativeArray<VertexAttributeDescriptor> vaArray = new NativeArray<VertexAttributeDescriptor>(
                numOfVertexAttributes, Allocator.Temp, NativeArrayOptions.UninitializedMemory
            );
            vaArray[0] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.Position,
                format: VertexAttributeFormat.Float32,
                dimension: 3
            );
            vaArray[1] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.Normal,
                format: VertexAttributeFormat.Float32,
                dimension: 3
            );
            vaArray[2] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.Tangent,
                format: VertexAttributeFormat.Float32,
                dimension: 4
            );
            vaArray[3] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.TexCoord0,
                format: VertexAttributeFormat.Float32,
                dimension: 2
            );
            data.SetVertexBufferParams(numOfVertices, vaArray);
            vaArray.Dispose();

            data.SetIndexBufferParams(numOfIndices, IndexFormat.UInt16);
            data.SetSubMesh(0,
                new SubMeshDescriptor(
                    0, 
                    numOfIndices, 
                    MeshTopology.Triangles){
                        bounds = b,
                        vertexCount = numOfVertices,
                },
            // We need to disable index checks here because we first set the submesh and 
            // we then fill the data in with vertex and index information. 
            // Check CreateAndLaunch() in ProcMesh.Burst.cs 
            // As for the bounds it's just optimization.
                MeshUpdateFlags.DontRecalculateBounds | MeshUpdateFlags.DontValidateIndices
                );
            stream0 = data.GetVertexData<StreamVertex>();
            // reinterpret index data as int3 to facilitate setting a triangle by vectorizing the call.
            //  i+0 => i.x
            //  i+1 => i.y
            //  i+2 => i.z
            indices = data.GetIndexData<ushort>().Reinterpret<utri16>(4);
        }

        // Always force inlining of this call as it is going to be more optimized vs
        // burst's automatic choice when we add conversions.
        // Small methods are inlined automatically and those that are used only one are also inlined.
        // Issue arises for code that is a bit more complex and is called often, like the case of this function
        // if it had conversions. 
        [MethodImpl(MethodImplOptions.AggressiveInlining)] 
        public void SetVertexBuffer(int index, VertexInfo v)
        {
            stream0[index] = new StreamVertex{
                position = v.position,
                normal = v.normal,
                tangent  = v.tangent,
                uv0 = v.uv0,
            };
        }
    }

    public struct MultiStream : IMeshStream
    {
        /// <summary>
        /// Positions
        /// </summary>
        [NativeDisableContainerSafetyRestriction]
        NativeArray<float3> stream0;

        /// <summary>
        /// Normals
        /// </summary>
        [NativeDisableContainerSafetyRestriction]
        NativeArray<float3> stream1;

        /// <summary>
        /// Tangents
        /// </summary>
        [NativeDisableContainerSafetyRestriction]
        NativeArray<float4> stream2;

        /// <summary>
        /// First set of UVs (UV0)
        /// </summary>
        [NativeDisableContainerSafetyRestriction]
        NativeArray<float2> stream3;

        [NativeDisableContainerSafetyRestriction]
        NativeArray<utri16> indices;

        private static readonly int numOfVertexAttributes = 4;
        public void SetTriangle(int index, int3 triangleIndices)
        {
            indices[index] = triangleIndices;
        }

        public void Setup(Mesh.MeshData data, int numOfVertices, int numOfIndices, Bounds b)
        {
            data.subMeshCount = 1;
            NativeArray<VertexAttributeDescriptor> vaArray = new NativeArray<VertexAttributeDescriptor>(
                numOfVertexAttributes, Allocator.Temp, NativeArrayOptions.UninitializedMemory
            );
            vaArray[0] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.Position,
                format: VertexAttributeFormat.Float32,
                dimension: 3,
                stream: 0
            );
            vaArray[1] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.Normal,
                format: VertexAttributeFormat.Float32,
                dimension: 3,
                stream: 1
            );
            vaArray[2] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.Tangent,
                format: VertexAttributeFormat.Float32,
                dimension: 4,
                stream: 2
            );
            vaArray[3] = new VertexAttributeDescriptor(
                attribute: VertexAttribute.TexCoord0,
                format: VertexAttributeFormat.Float32,
                dimension: 2,
                stream: 3
            );
            data.SetVertexBufferParams(numOfVertices, vaArray);
            vaArray.Dispose();

            data.SetIndexBufferParams(numOfIndices, IndexFormat.UInt16);
            data.SetSubMesh(0,
                new SubMeshDescriptor(
                    0, 
                    numOfIndices, 
                    MeshTopology.Triangles){
                        bounds = b,
                        vertexCount = numOfVertices,
                },
            // We need to disable index checks here because we first set the submesh and 
            // we then fill the data in with vertex and index information. 
            // Check CreateAndLaunch() in ProcMesh.Burst.cs 
            // As for the bounds it's just optimization.
                MeshUpdateFlags.DontRecalculateBounds | MeshUpdateFlags.DontValidateIndices
                );
            stream0 = data.GetVertexData<float3>();
            stream1 = data.GetVertexData<float3>(1);
            stream2 = data.GetVertexData<float4>(2);
            stream3 = data.GetVertexData<float2>(3);
            // reinterpret index data as int3 to facilitate setting a triangle by vectorizing the call.
            //  i+0 => i.x
            //  i+1 => i.y
            //  i+2 => i.z
            indices = data.GetIndexData<ushort>().Reinterpret<utri16>(2);
        }

        // Always force inlining of this call as it is going to be more optimized vs
        // burst's automatic choice when we add conversions.
        // Small methods are inlined automatically and those that are used only one are also inlined.
        // Issue arises for code that is a bit more complex and is called often, like the case of this function
        // if it had conversions. 
        [MethodImpl(MethodImplOptions.AggressiveInlining)] 
        public void SetVertexBuffer(int index, VertexInfo v)
        {
            stream0[index] = v.position;
            stream1[index] = v.normal;
            stream2[index] = v.tangent;
            stream3[index] = v.uv0;
        }
    }
}