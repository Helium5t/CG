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

    public bool multiStreamPacking = false;

    enum GeometryGenerator{
        SquareGrid,
        OptimizedSquareGrid
    }

    [SerializeField]
    GeometryGenerator geometryGenerator = GeometryGenerator.SquareGrid;

    ScheduleMeshJobDelegate[] singleStreamjobs ={
        MeshJob<SquareGrid, SingleStream>.CreateAndLaunch,
        MeshJob<OptimizedSquareGrid, SingleStream>.CreateAndLaunch,
    };
    ScheduleMeshJobDelegate[] multiStreamjobs ={
        MeshJob<SquareGrid, MultiStream>.CreateAndLaunch,
        MeshJob<OptimizedSquareGrid, MultiStream>.CreateAndLaunch,
    };

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

        ScheduleMeshJobDelegate geometryGeneratorLauncher;
        if (multiStreamPacking){
            geometryGeneratorLauncher =  singleStreamjobs[(int)geometryGenerator];
        }else{
            geometryGeneratorLauncher =  multiStreamjobs[(int)geometryGenerator];
        }
        JobHandle jh = geometryGeneratorLauncher(resolution, generatedMesh, md, default);
        jh.Complete();
        // Set mesh data
        ApplyAndDisposeWritableMeshData(mdArray, generatedMesh);
    }
}