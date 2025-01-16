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

        public int jobLength => resolution * resolution;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one);


        public void Execute<S>(int i, S stream) where S : struct, IMeshStream
        {
            // i in [0, resolution * resolution]
            // Get the lower left corner of the quad
            // Coordinates will go from 0 to resolution.
            // Each quad is size 1.
            // X is going to be each row so i % resolution
            // Y is going to be each column so i // resolution
            int quadZ = i / resolution,         // [0, resolution]
                quadX = i - resolution* quadZ; //  [0, resolution -1]
        
            // Keeps size fixed to 1 and centers plane in the center 
            float4 coord = float4(quadX, quadX +1f,quadZ, quadZ + 1f) / resolution - 0.5f;
            int iVertStart = 4*i;
            VertexInfo vi = new VertexInfo{
                position = float3(coord.x, 0f, coord.z),
                normal = float3(0f,1f,0f),
                tangent = float4(1f,0f,0f,-1f)
            };
            stream.SetVertexBuffer(iVertStart, vi);
            vi.position.x = coord.y;
            // uv coordinates will always go from 0 to 1, so we 
            // have to divide by resolution again, in order to 
            // get the section of uv to read.
            // For now we repeat the uv sample to see each quad
            vi.uv0.x = 1f;
            stream.SetVertexBuffer(iVertStart + 1,vi);
            vi.position.xz = coord.xw;
            vi.uv0.xy =  float2(0f,1f);
            stream.SetVertexBuffer(iVertStart + 2,vi);
            vi.position.xz = coord.yw;
            vi.uv0.xy = 1f;
            stream.SetVertexBuffer(iVertStart + 3,vi);
            
            int iIndexStart = 2 * i;
            stream.SetTriangle(
                iIndexStart, 
                iVertStart + int3(0,2,1)
                    );
            stream.SetTriangle(
                iIndexStart + 1, 
                iVertStart + int3(1,2,3)
                );
        }

    }
}