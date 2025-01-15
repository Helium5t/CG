using Unity.Collections;
using UnityEngine;
using UnityEngine.Rendering;

using Unity.Mathematics;

using static Unity.Mathematics.math;

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
public class AdvancedMultiStreamProceduralMesh : MonoBehaviour {

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
            dimension: 3,
            stream: 0
        );
        va[1] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.Normal,
            format: VertexAttributeFormat.Float32,
            dimension: 3,
            stream: 1
        );
        va[2] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.Tangent,
            format: VertexAttributeFormat.Float16,
            dimension: 4, // fourth dimension allows us to control "verso" without changing all components.
            stream: 2
        );
        va[3] = new VertexAttributeDescriptor(
            attribute:  VertexAttribute.TexCoord0,
            format: VertexAttributeFormat.Float16,
            dimension: 2,
            stream: 3
        );
        // Set mesh data as an array of 4 vertices, each with 4 attributes.
        meshData.SetVertexBufferParams(numberOfVertices,va);
        // uint16 = mathematics' ushort
        meshData.SetIndexBufferParams(numberOfTriangleIndices, IndexFormat.UInt16);
        va.Dispose(); 
        // GetVertexData always returns the first stream in the mesh data.
        // So it will retrieve positions just because we set the first stream to that.
        // It returns a pointer, there are no intermediary steps to save on resources.
        NativeArray<float3> pos = meshData.GetVertexData<float3>(/*0 by default*/);
        pos[0] = 0f;
        pos[1] = right();
        pos[2] = up();
        pos[3] = float3(1f,1f,0f);
        
		NativeArray<float3> normals = meshData.GetVertexData<float3>(1);
		normals[0] = normals[1] = normals[2] = normals[3] = back();

        half zeroH = half(0f), oneH = half(1f);

		NativeArray<half4> tangents = meshData.GetVertexData<half4>(2);
		tangents[0] = tangents[1] = tangents[2] = tangents[3] = half4(oneH, zeroH, zeroH, half(-1f));

		NativeArray<half2> texCoords = meshData.GetVertexData<half2>(3);
		texCoords[0] = zeroH;
		texCoords[1] = half2(oneH, zeroH);
		texCoords[2] = half2(zeroH, oneH);
		texCoords[3] = oneH;

        NativeArray<ushort> indices = meshData.GetIndexData<ushort>();
        indices[0] = 0;
        indices[1] = indices[4] = 2;
        indices[2] = indices[3] = 1;
        indices[5] = 3;



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