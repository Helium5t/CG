using ProcMesh;
using UnityEngine;
using static UnityEngine.Mesh;
using UnityEngine.Rendering;
using Unity.Jobs;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class MeshProcedural : MonoBehaviour {

    Mesh generatedMesh;

    public void Awake(){
        generatedMesh = new Mesh{
            name = "Procedural Mesh",
        };
        GenerateMesh();
        GetComponent<MeshFilter>().mesh = generatedMesh;
    }

    void GenerateMesh(){
        // Allocate space for one mesh
        MeshDataArray mdArray = AllocateWritableMeshData(1);

        MeshData md = mdArray[0];

        var jh = MeshJob<SquareGrid, SingleStream>.CreateAndLaunch(
            generatedMesh, md, default
        );

        jh.Complete();
        // Set mesh data
        ApplyAndDisposeWritableMeshData(mdArray, generatedMesh);
    }
}