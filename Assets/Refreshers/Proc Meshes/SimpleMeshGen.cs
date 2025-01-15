using UnityEngine;


[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class SimpleMeshGen : MonoBehaviour
{


    void OnEnable(){
        Mesh m = new Mesh{
            name = "ProcMesh",
        };
        m.vertices = new Vector3[]{
            Vector3.zero, Vector3.right, Vector3.up, 
            new Vector3(1f,1f),
        };
        // Clockwise due to culling principles
        m.triangles = new int[]{
            0,2,1, 1,2,3
        };
        // Must be same length as vertices array.
        m.normals = new Vector3[]{
            Vector3.back, Vector3.back, Vector3.back, 
			Vector3.back,
        };
        m.uv = new Vector2[] {
            Vector2.zero, Vector2.right, Vector2.up,
			Vector2.one
        };
        // Tangent vectors paired with normals to have
        // tangent to world space transformations for normal map.
        m.tangents = new Vector4[]{
			new Vector4(1f, 0f, 0f, -1f),
			new Vector4(1f, 0f, 0f, -1f),
			new Vector4(1f, 0f, 0f, -1f),
			new Vector4(1f, 0f, 0f, -1f),
        };


        GetComponent<MeshFilter>().mesh = m;
    }
}
