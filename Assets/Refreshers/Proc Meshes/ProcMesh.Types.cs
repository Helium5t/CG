using System.Runtime.InteropServices;
using Unity.Mathematics;


namespace ProcMesh{
     [StructLayout(LayoutKind.Sequential)]
    public struct utri16{
        public  ushort x;
        public  ushort y;
        public  ushort z;

        public static implicit operator utri16(int3 tri) =>new utri16(){
            x = (ushort)tri.x,
            y = (ushort)tri.y,
            z = (ushort)tri.z,
        };
    }
    public struct VertexInfo{
        public float3 position, normal;
        public float4 tangent;
        public float2 uv0;
    }
}