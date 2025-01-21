using ProcMesh;
using UnityEngine;
using static UnityEngine.Mesh;
using UnityEngine.Rendering;
using Unity.Jobs;
using Unity.VisualScripting;
using UnityEditor;
using UnityEditor.SearchService;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class MeshProcedural : MonoBehaviour {

    [Header("Debug values")]
    [SerializeField,Range(0.001f, 0.5f)]
    float gizmoSphereSize=0.01f;
    [SerializeField,Range(0.001f, 0.5f)]
    float gizmoVectorsSize=0.01f;

    [SerializeField]
    Color debugVertexColor = Color.cyan;

    [SerializeField]
    Color debugNormalColor = Color.cyan;

    [SerializeField]
    Color debugTangentColor = Color.cyan;

    [System.Flags]
    public enum DebugFlags {None = 0,  Vertices=1, Normals=0b10, Tangents=0b100}
    [SerializeField]
    DebugFlags activeGizmos = 0;


    public enum MaterialMode { Flat, Ripple}
    [Header("Geometry Parameters")]
    [SerializeField]
    MaterialMode choosenMat;

    [SerializeField]
    Material[] mats;


    [Header("Generation parameters")]
    [SerializeField, Range(0, 100)]
    int resolution = 1;

    Mesh generatedMesh;
    /// <summary>
    /// Used to visualize the values generated for the mesh
    /// </summary>
    Vector3[] vertices, normals;
    Vector4[] tangents;

    public bool multiStreamPacking = false;

    enum GeometryGenerator{
        SquarePlane,
        OptimizedSquarePlane,
        TrianglePlane,
        HexPlaneVertical, 
        HexPlaneHorizontal, 
        SphereFromSquarePlane,
    }


    [SerializeField]
    GeometryGenerator geometryGenerator = GeometryGenerator.SquarePlane;

    ScheduleMeshJobDelegate[] singleStreamjobs ={
        MeshJob<SquarePlane, SingleStream>.CreateAndLaunch,
        MeshJob<OptimizedSquarePlane, SingleStream>.CreateAndLaunch,
        MeshJob<TrianglePlane, SingleStream>.CreateAndLaunch,
        MeshJob<HexPlaneVertical, SingleStream>.CreateAndLaunch,
        MeshJob<HexPlaneHorizontal, SingleStream>.CreateAndLaunch,
        MeshJob<UVSphere, SingleStream>.CreateAndLaunch,
    };
    ScheduleMeshJobDelegate[] multiStreamjobs ={
        MeshJob<SquarePlane, MultiStream>.CreateAndLaunch,
        MeshJob<OptimizedSquarePlane, MultiStream>.CreateAndLaunch,
        MeshJob<TrianglePlane, MultiStream>.CreateAndLaunch,
        MeshJob<HexPlaneVertical, MultiStream>.CreateAndLaunch,
        MeshJob<HexPlaneHorizontal, MultiStream>.CreateAndLaunch,
        MeshJob<UVSphere, MultiStream>.CreateAndLaunch,
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

        if (activeGizmos != DebugFlags.None ){// Vertices are always used for any debug mode
            vertices = generatedMesh.vertices;
        } else if (vertices != null){
            vertices = null;
        }
        if ((DebugFlags.Normals & activeGizmos) != 0 ){
            normals = generatedMesh.normals;
        }else if (normals != null){
            normals = null;
        }
        if ((DebugFlags.Tangents & activeGizmos) != 0 ){
            tangents = generatedMesh.tangents;
        }else if (tangents != null){
            tangents = null;
        }

		GetComponent<MeshRenderer>().material = mats[(int)choosenMat];
    }

    void OnDrawGizmos(){
        if (generatedMesh == null || activeGizmos == DebugFlags.None){
            return;
        }
        for(int i=0; i< vertices.Length; i++){
            Vector3 vPos = transform.TransformPoint(vertices[i]);
            if ((DebugFlags.Vertices & activeGizmos) != 0 ){
                Gizmos.color = debugVertexColor;
                Gizmos.DrawSphere(vPos, gizmoSphereSize);
            }
            if ((DebugFlags.Normals & activeGizmos) != 0 ){
                Vector3 tNorm = transform.TransformDirection(normals[i]);
                Gizmos.color = debugNormalColor;
                Gizmos.DrawRay(vPos,tNorm*gizmoVectorsSize);
            }
            if ((DebugFlags.Tangents & activeGizmos) != 0 ){
                Vector3 tTang = transform.TransformDirection(tangents[i]);
                Gizmos.color = debugTangentColor;
                Gizmos.DrawRay(vPos, tTang*gizmoVectorsSize);
            }
        }
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