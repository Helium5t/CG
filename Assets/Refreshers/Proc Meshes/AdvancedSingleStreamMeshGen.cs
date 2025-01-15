using Unity.Collections;
using UnityEngine;
using UnityEngine.Rendering;

using Unity.Mathematics;

using System.Runtime.InteropServices;
using static Unity.Mathematics.math;
using System;

/// <summary>
/// Uses the advanced API 
/// operates in the native format of the mesh memory.
/// Mesh memory is logically split into regions, vertex and index.
/// Vertex regions is intended to contain sequential blocks of vertex data of the same format.
/// We can have up to 4 vertex data stream, we are going to use the 4 streams for:
///  - Vertices
///  - Normals
///  - Tangents
///  - UV Coordinates
///  in third order. 
/// </summary>
[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class AdvancedSingleStreamProceduralMesh : MonoBehaviour {

    /// <summary>
    /// StructLayout enforced sequential compilation of the struct.
    /// Otherwise Unity might change the order at compile time to optimize processes.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    struct VertexInfo{
        public float3 position, normal;
        public half4 tangent;
        public half2 uv0;
    }

	void OnEnable () {
        int numberOfTriangleIndices = 6; // 2 triangles x 3 vertices
        int numberOfVertices = 4;
        int numberOfVertexAttributes = 4; // coordinates, normals, tangents, texture coordinates
        // This is an array of meshdata, which is itself a struct that will be defined
        // via a descriptor later on.
        Mesh.MeshDataArray mda = Mesh.AllocateWritableMeshData(1); // amount of meshes to allocate. 

        Mesh.MeshData meshData = mda[0];

        // Temp allocation mandates the lifetime of the object is less than one frame.
        NativeArray<VertexAttributeDescriptor> va = new NativeArray<VertexAttributeDescriptor>(
            length: numberOfVertexAttributes, 
            Allocator.Temp,
            NativeArrayOptions.UninitializedMemory // Do not initialize the memory, as it's going to be filled right after.
        );
        va[0] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.Position,
            format: VertexAttributeFormat.Float32,
            dimension: 3
        );
        va[1] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.Normal,
            format: VertexAttributeFormat.Float32,
            dimension: 3
        );
        va[2] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.Tangent,
            format: VertexAttributeFormat.Float16,
            dimension: 4 // fourth dimension allows us to control "verso" without changing all components.
        );
        va[3] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.TexCoord0,
            format: VertexAttributeFormat.Float16,
            dimension: 2
        );
        // Set mesh data as an array of 4 vertices, each with 4 attributes.
        meshData.SetVertexBufferParams(numberOfVertices,va);
        va.Dispose(); 
        // GetVertexData always returns the first stream in the mesh data.
        // So it will retrieve positions just because we set the first stream to that.
        // It returns a pointer, there are no intermediary steps to save on resources.
        NativeArray<VertexInfo> pos = meshData.GetVertexData<VertexInfo>(/*0 by default*/);
        half h0 = half(0f), h1 = half(1f);
        VertexInfo vi = new VertexInfo{
            normal = back(),
            tangent = half4(h1,h0,h0, half(-1f)),
        };

        vi.position = 0f;
        vi.uv0 = h0;
        pos[0] = vi;
        
        vi.position = right();
        vi.uv0 = half2(h1,h0);
        pos[1] = vi;

        vi.position = up();
        vi.uv0 = half2(h0,h1);
        pos[2] = vi;

        vi.position = float3(1f,1f,0f);
        vi.uv0 = h1;
        pos[3] = vi;
        
        // uint16 = mathematics' ushort
        meshData.SetIndexBufferParams(numberOfTriangleIndices, IndexFormat.UInt16);
        // Define submeshes: only 1, made up of the two triangles
        meshData.subMeshCount = 1;
        // Manually define local space bounds and avoid recomuputation of them with
        // MeshUpdateFlags. we don't expect the vertices to move beyond those bounds
        // in local space at any point.
        Bounds b = new Bounds(new Vector3(0.5f, 0.5f), new Vector3(1f,1f));
        meshData.SetSubMesh(0, new SubMeshDescriptor(
            0, 
            numberOfTriangleIndices,
            MeshTopology.Triangles
        ){
            bounds = b,
            vertexCount = numberOfVertices,
        }, MeshUpdateFlags.DontRecalculateBounds);

		var mesh = new Mesh {
			name = "Procedural Mesh"
		};





        // Bind the mesh data to the mesh object. Similar to the compute buffer paradigm.
        Mesh.ApplyAndDisposeWritableMeshData(
            mda, mesh
        );

		GetComponent<MeshFilter>().mesh = mesh;
	}
}