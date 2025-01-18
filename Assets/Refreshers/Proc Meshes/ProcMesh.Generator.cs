using Unity.Jobs;
using Unity.Mathematics;
using Unity.VisualScripting;
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

    public struct SquarePlane : IMeshGenerator
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
    /// <summary>
    /// Optimized version of SquareGrid that generates a plane of quads.
    /// Vertices in the same position are only generated once. 
    /// </summary>
    public struct OptimizedSquarePlane : IMeshGenerator
    {
        public int verticesCount => (resolution+1) * (resolution+1);

        public int indicesCount => 6 * resolution * resolution;

        public int jobLength => resolution + 1;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one);


        // Each execute takes care of a row of quads from x=0 to x=1
        public void Execute<S>(int z, S stream) where S : struct, IMeshStream
        {
            float z01 = (float)z/resolution;
            int vertIdx = (resolution+1)*z,
                triIdx = 2*resolution*(z-1);
            VertexInfo vi = new VertexInfo{
                position = float3(
                    -0.5f, 0f, z01 - 0.5f
                ),
                normal = float3(0f,1f,0f),
                tangent = float4(1f,0f,0f,-1f)
            };
            // Loop unrolling so we can vectorize 
            // triangle and vertex generation together.
            vi.uv0.y = z01;
            stream.SetVertexBuffer(vertIdx, vi);
            vertIdx++;
            for(int i=1; i<= resolution; i++, vertIdx++, triIdx +=2){
                float i01 = (float)i/resolution;
                vi.position.x = i01 - 0.5f;
                vi.uv0.x = i01;
                stream.SetVertexBuffer(vertIdx, vi);
                if (z> 0){ // skip first row
                stream.SetTriangle(
                    triIdx,
                    vertIdx + int3(-resolution-1,-1,0)
                );
                stream.SetTriangle(
                    triIdx+1,
                    vertIdx + int3(-resolution-2,-1,-resolution-1)
                );
                }
            }
        }

    }


    public struct TrianglePlane : IMeshGenerator
    {
        public int verticesCount => (resolution+1) * (resolution+1);

        public int indicesCount => 6 * resolution * resolution;

        public int jobLength => resolution + 1;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one);

        private static readonly float rad3halved = 0.86602540378f;
        
        // Each execute takes care of a row of quads from x=0 to x=1
        public void Execute<S>(int z, S stream) where S : struct, IMeshStream
        {
            float z01 = (float)z*rad3halved/resolution;
            float offsetOdd = select(0.5f/resolution, 0f, (z & 1) == 0);
            int vertIdx = (resolution+1)*z,
                triIdx = 2*resolution*(z-1);
            
            int3 triangleOffsetOne = select(
                int3(-resolution-1,-1,0),
                int3(-resolution-2,-1,0),
                (z&1) == 0
            ) ,
            triangleOffsetTwo = select(
                int3(-resolution-2,-1,-resolution-1),
                int3(-resolution-2, 0,-resolution-1),
                (z&1) == 0
            );
            VertexInfo vi = new VertexInfo{
                position = float3(
                   offsetOdd -0.5f, 0f, z01 - 0.5f 
                ),
                normal = float3(0f,1f,0f),
                tangent = float4(1f,0f,0f,-1f)
            };
            // Loop unrolling so we can vectorize 
            // triangle and vertex generation together.
            vi.uv0.y = z01;
            vi.uv0.x += offsetOdd;
            stream.SetVertexBuffer(vertIdx, vi);
            vertIdx++;
            for(int i=1; i<= resolution; i++, vertIdx++, triIdx +=2){
                float i01 = (float)i/resolution;
                vi.position.x = i01 - 0.5f + offsetOdd;
                vi.uv0.x = i01 + offsetOdd;
                stream.SetVertexBuffer(vertIdx, vi);
                if (z> 0){ // skip first row
                stream.SetTriangle(
                    triIdx,
                    vertIdx + triangleOffsetOne
                );
                stream.SetTriangle(
                    triIdx+1,
                    vertIdx + triangleOffsetTwo
                );
                }
            }
        }

    }
}