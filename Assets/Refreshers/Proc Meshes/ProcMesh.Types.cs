using System.Runtime.InteropServices;
using Unity.Mathematics;


namespace ProcMesh{
     [StructLayout(LayoutKind.Sequential)]
    public struct Uint16Tri{
        public  ushort x;
        public  ushort y;
        public  ushort z;

        public static implicit operator Uint16Tri(int3 tri) =>new Uint16Tri(){
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