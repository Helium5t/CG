using Unity.Jobs;
using Unity.Mathematics;
using UnityEngine;
using static Unity.Mathematics.math;

namespace ProcMesh{

    public interface IMeshGenerator{

        int verticesCount {get;}
        int indicesCount {get;}
        int jobLength {get;}
        int resolution{get;set;}
        Bounds bounds {get;}

        /// <summary>
        /// Generate mesh info for index i and pack them using stream S
        /// The result will be inside the S.VertexInfo 
        /// </summary>
        void Execute<S>(int i, S stream) where S: struct, IMeshStream;
    }

    public struct SquareGrid : IMeshGenerator
    {
        public int verticesCount => 4 * resolution * resolution;

        public int indicesCount => 6 * resolution * resolution;

        public int jobLength => resolution;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one);


        // Each execute takes care of a row of quads from x=0 to x=1
        public void Execute<S>(int z, S stream) where S : struct, IMeshStream
        {
            // z goes from 0 to resolution, so we can use that to "skip"
            // and let the previous index generate the row up until 
            // the end of the row.
            int iVertStart = 4 * resolution * z;
            int iIndexStart = 2 * z * resolution;


            // optimization to leverage z never changing
            float2 cz = float2(z, z+1f) / resolution - 0.5f;


            // z in [0, resolution]
            // Get the lower left corner of the quad
            // Coordinates will go 
            // z = z
            // x from 0 to resolution
            for (int x = 0; x < resolution; x++){
                int quadX = x, quadZ = z;
            // Keeps size fixed to 1 and centers plane in the center 
            float2 cx = float2(x, x+1f)/resolution - 0.5f;
            VertexInfo vi = new VertexInfo{
                position = float3(cx.x, 0f, cz.x),
                normal = float3(0f,1f,0f),
                tangent = float4(1f,0f,0f,-1f)
            };
            stream.SetVertexBuffer(iVertStart, vi);
            vi.position.x = cx.y;
            // uv coordinates will always go from 0 to 1, so we 
            // have to divide by resolution again, in order to 
            // get the section of uv to read.
            // For now we repeat the uv sample to see each quad
            vi.uv0.x = 1f;
            stream.SetVertexBuffer(iVertStart + 1,vi);
            vi.position.x = cx.x;
            vi.position.z = cz.y;
            vi.uv0.xy =  float2(0f,1f);
            stream.SetVertexBuffer(iVertStart + 2,vi);
            vi.position.x = cx.y;
            vi.position.z = cz.y;
            vi.uv0.xy = 1f;
            stream.SetVertexBuffer(iVertStart + 3,vi);
            
            stream.SetTriangle(
                iIndexStart, 
                iVertStart + int3(0,2,1)
                    );
            stream.SetTriangle(
                iIndexStart + 1, 
                iVertStart + int3(1,2,3)
                );
            // Increase index starts for the next quad
            iVertStart += 4;
            iIndexStart += 2;
            }
        }

    }
}