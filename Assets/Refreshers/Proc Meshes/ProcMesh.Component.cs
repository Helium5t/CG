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
        SquarePlane,
        OptimizedSquarePlane,
        TrianglePlane,
        HexPlaneVertical, 
        HexPlaneHorizontal, 
    }

    [SerializeField]
    GeometryGenerator geometryGenerator = GeometryGenerator.SquarePlane;

    ScheduleMeshJobDelegate[] singleStreamjobs ={
        MeshJob<SquarePlane, SingleStream>.CreateAndLaunch,
        MeshJob<OptimizedSquarePlane, SingleStream>.CreateAndLaunch,
        MeshJob<TrianglePlane, SingleStream>.CreateAndLaunch,
        MeshJob<HexPlaneVertical, SingleStream>.CreateAndLaunch,
        MeshJob<HexPlaneHorizontal, SingleStream>.CreateAndLaunch,
    };
    ScheduleMeshJobDelegate[] multiStreamjobs ={
        MeshJob<SquarePlane, MultiStream>.CreateAndLaunch,
        MeshJob<OptimizedSquarePlane, MultiStream>.CreateAndLaunch,
        MeshJob<TrianglePlane, MultiStream>.CreateAndLaunch,
        MeshJob<HexPlaneVertical, MultiStream>.CreateAndLaunch,
        MeshJob<HexPlaneHorizontal, MultiStream>.CreateAndLaunch,
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
            geometryGeneratorLauncher =  multiStreamjobs[(int)geometryGenerator];
        }else{
            geometryGeneratorLauncher =  singleStreamjobs[(int)geometryGenerator];
        }
        JobHandle jh = geometryGeneratorLauncher(resolution, generatedMesh, md, default);
        jh.Complete();
        // Set mesh data
        ApplyAndDisposeWritableMeshData(mdArray, generatedMesh);
    }
}