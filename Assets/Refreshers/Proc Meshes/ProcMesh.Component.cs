using ProcMesh;
using UnityEngine;
using static UnityEngine.Mesh;
using UnityEngine.Rendering;
using Unity.Jobs;
using Unity.VisualScripting;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class MeshProcedural : MonoBehaviour {

    Mesh generatedMesh;

    [SerializeField, Range(0, 100)]
    int resolution = 1;

    public void OnValidate(){
        this.enabled = true;
    }

    public void Awake(){
        generatedMesh = new Mesh{
            name = "Procedural Mesh",
        };
        GetComponent<MeshFilter>().mesh = generatedMesh;
    }

    public void Update(){
        GenerateMesh();
        this.enabled = false;
    }

    void GenerateMesh(){
        // Allocate space for one mesh
        MeshDataArray mdArray = AllocateWritableMeshData(1);

        MeshData md = mdArray[0];

        var jh = MeshJob<SquareGrid, MultiStream>.CreateAndLaunch(
            resolution, generatedMesh, md, default
        );

        jh.Complete();
        // Set mesh data
        ApplyAndDisposeWritableMeshData(mdArray, generatedMesh);
    }
}