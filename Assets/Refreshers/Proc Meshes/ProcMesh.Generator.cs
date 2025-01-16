using Unity.Jobs;
using Unity.Mathematics;
using UnityEngine;
using static Unity.Mathematics.math;

namespace ProcMesh{

    public interface IMeshGenerator{

        int verticesCount {get;}
        int indicesCount {get;}
        int jobLength {get;}

        Bounds bounds {get;}

        /// <summary>
        /// Generate mesh info for index i and pack them using stream S
        /// The result will be inside the S.VertexInfo 
        /// </summary>
        void Execute<S>(int i, S stream) where S: struct, IMeshStream;
    }

    public struct SquareGrid : IMeshGenerator
    {
        public int verticesCount => 4;

        public int indicesCount => 6;

        public int jobLength => 1;

        public Bounds bounds => new Bounds(Vector3.one * 0.5f, Vector3.one);

        public void Execute<S>(int i, S stream) where S : struct, IMeshStream
        {
            VertexInfo vi = new VertexInfo{
                normal = float3(0f,0f,1f),
                tangent = float4(1f,0f,0f,-1f)
            };
            stream.SetVertexBuffer(0, vi);
            vi.position.x = 1f;
            vi.uv0.x = 1f;
            stream.SetVertexBuffer(1,vi);
            vi.position.xy = float2(0f,1f);
            vi.uv0.xy =  float2(0f,1f);
            stream.SetVertexBuffer(2,vi);
            vi.position.xy = 1f;
            vi.uv0.xy = 1f;
            stream.SetVertexBuffer(3,vi);

            stream.SetTriangle(0, int3(0,2,1));
            stream.SetTriangle(1, int3(1,2,3));
        }

    }
}