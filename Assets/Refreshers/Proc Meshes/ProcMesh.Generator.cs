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
                    iVertStart + int3(0,2,3)
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


    /// <summary>
    /// Unoptimized generation of a plane that uses exagon cells. <br \>
    /// Vertices are duplicated. <br \>
    /// Generates 7 times the resolution squared of vertices. 
    /// </summary>
    public struct HexPlaneVertical : IMeshGenerator
    {
        public int verticesCount => 7 * resolution * resolution;

        public int indicesCount => 18 * resolution * resolution;

        public int jobLength => resolution;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one);

        /// <summary>
        /// sqrt(3)/4 (because we use a half-height triangle in each hex to keep it's diameter 1)
        /// </summary>
        private static readonly float triHeight = 0.86602540378f/2f;

        public void Execute<S>(int z, S stream) where S : struct, IMeshStream
        {
            int iVertStart = 7 * resolution * z;
            int iIndexStart = 6 * z * resolution;

            float2 hexOffset = 0f;

            if (resolution > 1){
                hexOffset.x = (((z&1) == 0?0.5f:1.5f)-resolution)*triHeight; 
                hexOffset.y = -0.375f * (resolution - 1);
            }


            // leverage z not changing wrt the center of the hex


            // z in [0, resolution]
            // Get the lower left corner of the quad
            // Coordinates will go 
            // z = z
            // x from 0 to resolution
            for (int x = 0; x < resolution; x++){
                // First vertex is center of hexagon
                float2 hexCenter = (float2(2f* triHeight * x, 0.75f * z)+hexOffset) / resolution;
                VertexInfo vi = new VertexInfo{
                    position = float3(hexCenter.x, 0f, hexCenter.y),
                    normal = float3(0f,1f,0f),
                    tangent = float4(1f,0f,0f,-1f),
                    uv0 = 0.5f,
                };
                stream.SetVertexBuffer(iVertStart, vi);
			    var cz = hexCenter.y + float4(-0.5f, -0.25f, 0.25f, 0.5f) / resolution;
                // Keeps size fixed to 1 and centers hex plane in the center
                float2 cx = hexCenter.x +  float2(-triHeight, triHeight)/ resolution;

                // define other vertices in clockwise order starting from the lower 
                // vertex (think of an hexagon standing on a vertex)
                vi.position.z = cz.x;
                vi.uv0.y = 0f;
                stream.SetVertexBuffer(iVertStart + 1,vi);
                
                vi.position.x = cx.x;
                vi.position.z = cz.y;
				vi.uv0 = float2(0.5f - triHeight, 0.25f);
                stream.SetVertexBuffer(iVertStart + 2,vi);

                vi.position.z = cz.z;
                vi.uv0.y = 0.75f;
                stream.SetVertexBuffer(iVertStart + 3,vi);

                vi.position.x = hexCenter.x;
                vi.position.z = cz.w;
				vi.uv0 = float2(0.5f,1f);
                stream.SetVertexBuffer(iVertStart + 4,vi);

                vi.position.x = cx.y;
                vi.position.z = cz.z;
				vi.uv0 = float2(0.5f + triHeight, 0.75f);
                stream.SetVertexBuffer(iVertStart + 5,vi);

                vi.position.z = cz.y;
                vi.uv0.y = 0.25f;
                stream.SetVertexBuffer(iVertStart + 6,vi);
                
                stream.SetTriangle(
                    iIndexStart, 
                    iVertStart + int3(0,1,2)
                        );
                stream.SetTriangle(
                    iIndexStart + 1, 
                    iVertStart + int3(0,2,3)
                    );
                stream.SetTriangle(
                    iIndexStart + 2, 
                    iVertStart + int3(0,3,4)
                    );
                stream.SetTriangle(
                    iIndexStart + 3, 
                    iVertStart + int3(0,4,5)
                    );
                stream.SetTriangle(
                    iIndexStart + 4, 
                    iVertStart + int3(0,5,6)
                    );
                stream.SetTriangle(
                    iIndexStart + 5, 
                    iVertStart + int3(0,6,1)
                    );
                // Increase index starts for the next quad
                iVertStart += 7;
                iIndexStart += 6;
            }
        }

    }
    /// <summary>
    /// Same as HexPlaneVertical, but hexagon lies on a side.
    /// </summary>
    public struct HexPlaneHorizontal : IMeshGenerator
    {
        public int verticesCount => 7 * resolution * resolution;

        public int indicesCount => 18 * resolution * resolution;

        public int jobLength => resolution;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one);

        /// <summary>
        /// sqrt(3)/4 (because we use a half-height triangle in each hex to keep it's diameter 1)
        /// </summary>
        private static readonly float triHeight = 0.86602540378f/2f;

        public void Execute<S>(int x, S stream) where S : struct, IMeshStream
        {
            int iVertStart = 7 * resolution * x;
            int iIndexStart = 6 * x * resolution;

            float2 hexOffset = 0f;

            if (resolution > 1){
                hexOffset.x = -0.375f * (resolution - 1);
                hexOffset.y = (((x&1) == 0?0.5f:1.5f)-resolution)*triHeight; 
            }


            // leverage z not changing wrt the center of the hex


            // z in [0, resolution]
            // Get the lower left corner of the quad
            // Coordinates will go 
            // z = z
            // x from 0 to resolution
            for (int z = 0; z < resolution; z++){
                // First vertex is center of hexagon
                float2 hexCenter = (float2(0.75f * x, 2f * triHeight * z)+hexOffset) / resolution;
                VertexInfo vi = new VertexInfo{
                    position = float3(hexCenter.x, 0f, hexCenter.y),
                    normal = float3(0f,1f,0f),
                    tangent = float4(1f,0f,0f,-1f),
                    uv0 = 0.5f,
                };
                stream.SetVertexBuffer(iVertStart, vi);
			    float4 cx = hexCenter.x + float4(-0.5f, -0.25f, 0.25f, 0.5f) / resolution;
                // Keeps size fixed to 1 and centers hex plane in the center
                float2 cz = hexCenter.y +  float2(triHeight, -triHeight)/ resolution;

                // define other vertices in clockwise order starting from the lower 
                // vertex (think of an hexagon standing on a vertex)
                vi.position.x = cx.x;
                vi.uv0.x = 0f;
                stream.SetVertexBuffer(iVertStart + 1,vi);
                
                vi.position.x = cx.y;
                vi.position.z = cz.x;
				vi.uv0 = float2(0.25f,0.5f + triHeight);
                stream.SetVertexBuffer(iVertStart + 2,vi);

                vi.position.x = cx.z;
                vi.uv0.x = 0.75f;
                stream.SetVertexBuffer(iVertStart + 3,vi);

                vi.position.x = cx.w;
                vi.position.z = hexCenter.y;
				vi.uv0 = float2(1f,0.5f);
                stream.SetVertexBuffer(iVertStart + 4,vi);

                vi.position.x = cx.z;
                vi.position.z = cz.y;
				vi.uv0 = float2(0.75f,0.5f - triHeight);
                stream.SetVertexBuffer(iVertStart + 5,vi);

                vi.position.x = cx.y;
                vi.uv0.x = 0.25f;
                stream.SetVertexBuffer(iVertStart + 6,vi);
                
                stream.SetTriangle(
                    iIndexStart, 
                    iVertStart + int3(0,1,2)
                        );
                stream.SetTriangle(
                    iIndexStart + 1, 
                    iVertStart + int3(0,2,3)
                    );
                stream.SetTriangle(
                    iIndexStart + 2, 
                    iVertStart + int3(0,3,4)
                    );
                stream.SetTriangle(
                    iIndexStart + 3, 
                    iVertStart + int3(0,4,5)
                    );
                stream.SetTriangle(
                    iIndexStart + 4, 
                    iVertStart + int3(0,5,6)
                    );
                stream.SetTriangle(
                    iIndexStart + 5, 
                    iVertStart + int3(0,6,1)
                    );
                // Increase index starts for the next quad
                iVertStart += 7;
                iIndexStart += 6;
            }
        }

    }

    /// <summary>
    /// Sphere starting from a square grid. Due to the wrapping of a constant size flat plane,
    /// the point distribution will be uneven once wrapped.
    /// Given a sphere cannot have less than 4 quadrants and 2 hemisphere, minimum resolution is 8.
    /// </summary>
    public struct UVSphere : IMeshGenerator
    {
        // The seam line does not generate poles => -2
        public int verticesCount => (latResolution+1) * (longResolution+1)-2;

        public int indicesCount => 6 * latResolution * (longResolution-1/*Due to last row being triangles not quads*/);

        public int jobLength => latResolution + 1;

        private int latResolution => resolution*4;
        private int longResolution => resolution *2;

        public int resolution { get; set; }
        
        // Size of bound is 2 as geometry is computed on r=1 
        public Bounds bounds => new Bounds(Vector3.zero, Vector3.one * 2f);

         public void Execute<S>(int u, S stream) where S : struct, IMeshStream{
            if (u == 0){
                GenerateSeam(stream);
            }else{
                GenerateStrip(u,stream);
            }
         }

        // Each execute takes care of a row of quads from x=0 to x=1
        public void GenerateStrip<S>(int u, S stream) where S : struct, IMeshStream
        {
            // The second vertical line connects to the seam (the previous and first line).
            // It has no pole vertices, so we need to account that by going forward by one with
            // the index 
            int prevRowIdx = (u==1?0:-1)-longResolution;
            float u01 = (float)u/latResolution;
            int vertIdx = (longResolution+1)*u - 2,
                triIdx = 2*(longResolution-1)*(u-1);

            VertexInfo vi = new VertexInfo{
                position = float3(0f,-1f,0f),
                normal = float3(0f,-1f,0f),
                tangent = float4(1f,0f,0f,1f)
            };
            sincos(
                2f * PI * (u-0.5f)/latResolution, 
                out vi.tangent.z, 
                out vi.tangent.x
            );
            vi.tangent.w = -1f;
            // Loop unrolling so we can vectorize 
            // triangle and vertex generation together.
            vi.uv0.x = (u-0.5f)/ latResolution;
            stream.SetVertexBuffer(vertIdx, vi);
            // Set north pole
            vi.position.y = vi.normal.y = vi.uv0.y = 1f;
            stream.SetVertexBuffer(vertIdx + longResolution, vi);

            vertIdx++;

            // Set triangles
            stream.SetTriangle(
                triIdx,
                vertIdx + int3(-1, prevRowIdx, 0)
            );
            triIdx++;

            float3 circleCoords = 0f;
            sincos( 
                2f * PI * u01,
                out circleCoords.x,
                out circleCoords.z
            );
            // Reset parameters for not-pole vertices
            vi.uv0.x = u01; 
            vi.tangent = float4(circleCoords.z,0f,circleCoords.x, 1f);
            circleCoords.z *= -1f;
            for(int v=1; v< longResolution; v++, vertIdx++){
                float v01 = (float)v/longResolution;

                sincos(
                    PI - PI * v01, 
                    out float s, 
                    out float py
                );
                vi.position  = circleCoords * s;
                vi.position.y = py;
                vi.normal = normalize(vi.position);
                vi.uv0.y = v01;
                stream.SetVertexBuffer(vertIdx, vi);

                // Set triangles
                if (v >1){
                    stream.SetTriangle(
                        triIdx,
                        vertIdx + int3(-1, prevRowIdx, 0)
                    );
                    stream.SetTriangle(
                        triIdx+1,
                        vertIdx + int3(prevRowIdx-1, prevRowIdx, -1)
                    );
                    triIdx +=2;
                }
            }
            // Loop unrolling because we took out north pole
            stream.SetTriangle(
                triIdx,
                vertIdx + int3(prevRowIdx-1, 0, -1)
            );
        }

        // Generates first line of vertices. U = 0
        // Does not generate triangles
        
        public void GenerateSeam<S>(S stream) where S : struct, IMeshStream
        {

            VertexInfo vi = new VertexInfo{
                tangent = float4(1f,0f,0f,1f)
            };
            // Reset values for non-pole vertices
            for(int v=1; v< longResolution; v++){
                float v01 = (float)v/longResolution;
                sincos(
                    // Starting from 180 and decreasing keeps the same sin and inverts cos values
                    PI - PI * v01,
                    out float s,
                    out vi.position.y
                );
                vi.position.z  = -s;
                // No need for next line due to PI - ....
                // vi.position.y *= -1f;
                vi.normal = normalize(vi.position);
                vi.uv0.y = v01;
                // when u =0, north and south pole will be v = 0 and v = longResolution
                // so in this cycle v =[1..longRes) = vertIdx. No need for vertIdx
                stream.SetVertexBuffer(v-1, vi);
            }
        }

    }


    public struct SphereFromCube : IMeshGenerator
    {

        /// <summary>
        /// Represent the side of the cube.
        /// side origin is the origin of the cube side.
        /// uAxis and vAxis represent the vector that goes from 0 to 1 along the u and v axis, respectively.
        /// Will essentially dictate the direction and total side size along which to generate the quads.
        /// </summary>
        struct Side {
            public int id;
            public float3 origin,
            uAxis,
            vAxis,
            normal;
            public float4 tangent; 
        };

        public int verticesCount => 6 * 4 * resolution * resolution;

        public int indicesCount => 6 * 6 * resolution * resolution;

        public int jobLength => 6 * resolution;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero , Vector3.one * 2f);


        /*
        Normal vectors for each side (and thus where they are facing)
        0 = back
        1 = right
        2 = down
        3 = front
        4 = left
        5 = up
        */
        static Side GetSide(int id) =>id switch{ 
            0 => new Side{
                id = id,
                origin = -1f,
                uAxis = 2f * right(),
                vAxis = 2f * up(),
            },
            1 => new Side{
                id = id,
                origin = float3(1f, -1f, -1f),
                uAxis = 2f * forward(),
                vAxis = 2f * up(),
            },
            2 => new Side{
                id = id,
                origin = -1f,
                uAxis = 2f * forward(),
                vAxis = 2f * right(),
            },
            3 => new Side{
                id = id,
                origin = float3(-1f, -1f, 1f),
                uAxis = 2f * up(),
                vAxis = 2f * right(),
            },
            4 => new Side{
                id = id,
                origin = -1f,
                uAxis = 2f * up(),
                vAxis = 2f * forward(),
            },
            5 => new Side{
                id = id,
                origin = float3(-1f, 1f, -1f),
                uAxis = 2f * right(),
                vAxis = 2f * forward(),
            },
            _ => new Side{
                id = id,
                origin = float3(1f, -1f, -1f),
                uAxis = 2f * forward(),
                vAxis = 2f * up(),
            }
        };

        static float3 CubeToSphere (float3 p) => normalize(p);

        // Each execute takes care of a row of quads from x=0 to x=1
        // The side will also change based on the value of i.
        public void Execute<S>(int i, S stream) where S : struct, IMeshStream
        {

            /*
            Normal vectors for each side (and thus where they are facing)
            0 = back
            1 = right
            2 = front
            3 = left
            4 = down
            5 = up
            */
            int u = i/6;
            var side = GetSide(i - 6 *u);
            int iVertStart = 4 * resolution * (resolution * side.id + u);
            int iIndexStart = 2 * resolution * (resolution * side.id + u);

            float3 cuCur, cuNext;
            cuCur = side.uAxis * u/resolution;
            cuNext =  side.uAxis * (u+1)/ resolution;

            // u in [0, resolution]
            // Get the lower left corner of the quad
            // Coordinates will go 
            // u = u
            // v from 0 to resolution
            for (int v = 0; v < resolution; v++){
                float3 cvCur = side.vAxis * v / resolution;
                float3 cvNext = side.vAxis * (v+1) / resolution;     

                // Bottom left corner of quad
                float3 pBL   =  CubeToSphere(side.origin + cuCur + cvCur),
                // Bottom right corner of quad
                pBR = CubeToSphere( side.origin  + cuNext + cvCur),
                // Top left corner
                pTL = CubeToSphere(side.origin + cuCur + cvNext),
                // Top right corner
                pTR = CubeToSphere(side.origin + cuNext + cvNext);
                
                VertexInfo vi = new VertexInfo{
                    position = pBL,
                    normal = pBL,
                    tangent = float4(normalize(pBR-pBL),-1f)
                };
                stream.SetVertexBuffer(iVertStart, vi);

                vi.position = vi.normal = pBR;
                // vi.tangent.xyz = normalize(pNext-pCur);
                vi.uv0.x = 1f;
                stream.SetVertexBuffer(iVertStart + 1,vi);

                vi.position = vi.normal = pTL;
                vi.tangent.xyz = normalize(pTR-pTL);
                vi.uv0.xy =  float2(0f,1f);
                stream.SetVertexBuffer(iVertStart + 2,vi);

                vi.position = vi.normal = pTR;
                // vi.tangent = float4(normalize(pNext-pCur),1f);
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

    public struct UniformSphereFromCube : IMeshGenerator
    {

        /// <summary>
        /// Represent the side of the cube.
        /// side origin is the origin of the cube side.
        /// uAxis and vAxis represent the vector that goes from 0 to 1 along the u and v axis, respectively.
        /// Will essentially dictate the direction and total side size along which to generate the quads.
        /// </summary>
        struct Side {
            public int id;
            public float3 origin,
            uAxis,
            vAxis,
            normal;
            public float4 tangent; 
        };

        public int verticesCount => 6 * 4 * resolution * resolution;

        public int indicesCount => 6 * 6 * resolution * resolution;

        public int jobLength => 6 * resolution;

        public int resolution { get; set; }
        
        public Bounds bounds => new Bounds(Vector3.zero , Vector3.one * 2f);


        /*
        Normal vectors for each side (and thus where they are facing)
        0 = back
        1 = right
        2 = down
        3 = front
        4 = left
        5 = up
        */
        static Side GetSide(int id) =>id switch{ 
            0 => new Side{
                id = id,
                origin = -1f,
                uAxis = 2f * right(),
                vAxis = 2f * up(),
            },
            1 => new Side{
                id = id,
                origin = float3(1f, -1f, -1f),
                uAxis = 2f * forward(),
                vAxis = 2f * up(),
            },
            2 => new Side{
                id = id,
                origin = -1f,
                uAxis = 2f * forward(),
                vAxis = 2f * right(),
            },
            3 => new Side{
                id = id,
                origin = float3(-1f, -1f, 1f),
                uAxis = 2f * up(),
                vAxis = 2f * right(),
            },
            4 => new Side{
                id = id,
                origin = -1f,
                uAxis = 2f * up(),
                vAxis = 2f * forward(),
            },
            5 => new Side{
                id = id,
                origin = float3(-1f, 1f, -1f),
                uAxis = 2f * right(),
                vAxis = 2f * forward(),
            },
            _ => new Side{
                id = id,
                origin = float3(1f, -1f, -1f),
                uAxis = 2f * forward(),
                vAxis = 2f * up(),
            }
        };

        static float3 CubeToSphere (float3 p) => p * sqrt(
			1f - ((p * p).yxx + (p * p).zzy) / 2f + (p * p).yxx * (p * p).zzy / 3f
		);

        // Each execute takes care of a row of quads from x=0 to x=1
        // The side will also change based on the value of i.
        public void Execute<S>(int i, S stream) where S : struct, IMeshStream
        {

            /*
            Normal vectors for each side (and thus where they are facing)
            0 = back
            1 = right
            2 = front
            3 = left
            4 = down
            5 = up
            */
            int u = i/6;
            var side = GetSide(i - 6 *u);
            int iVertStart = 4 * resolution * (resolution * side.id + u);
            int iIndexStart = 2 * resolution * (resolution * side.id + u);

            float3 cuCur, cuNext;
            cuCur = side.uAxis * u/resolution;
            cuNext =  side.uAxis * (u+1)/ resolution;

            // u in [0, resolution]
            // Get the lower left corner of the quad
            // Coordinates will go 
            // u = u
            // v from 0 to resolution
            for (int v = 0; v < resolution; v++){
                float3 cvCur = side.vAxis * v / resolution;
                float3 cvNext = side.vAxis * (v+1) / resolution;     

                // Bottom left corner of quad
                float3 pBL   =  CubeToSphere(side.origin + cuCur + cvCur),
                // Bottom right corner of quad
                pBR = CubeToSphere( side.origin  + cuNext + cvCur),
                // Top left corner
                pTL = CubeToSphere(side.origin + cuCur + cvNext),
                // Top right corner
                pTR = CubeToSphere(side.origin + cuNext + cvNext);
                
                VertexInfo vi = new VertexInfo{
                    position = pBL,
                    normal = pBL,
                    tangent = float4(normalize(pBR-pBL),-1f)
                };
                stream.SetVertexBuffer(iVertStart, vi);

                vi.position = vi.normal = pBR;
                // vi.tangent.xyz = normalize(pNext-pCur);
                vi.uv0.x = 1f;
                stream.SetVertexBuffer(iVertStart + 1,vi);

                vi.position = vi.normal = pTL;
                vi.tangent.xyz = normalize(pTR-pTL);
                vi.uv0.xy =  float2(0f,1f);
                stream.SetVertexBuffer(iVertStart + 2,vi);

                vi.position = vi.normal = pTR;
                // vi.tangent = float4(normalize(pNext-pCur),1f);
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