using System.Numerics;
using Unity.Burst;
using Unity.Burst.Intrinsics;
using Unity.Collections;
using Unity.Jobs;
using Unity.Mathematics;

using static Unity.Mathematics.math;


// Generates the position coordinates for different shapes (Torus, Sphere etc....)
public static class ShapeGen {
    // Contains a set of 4 points and 4 normals. (for vectorization purposes)
    public struct PackedPointInfo {
        public float4x3 pos, normals;
    }

    public interface IShapeGenerator{
        PackedPointInfo GeneratePoints(int i , float resolution, float invResolution);
    }

    public delegate JobHandle ScheduleDelegate (
        NativeArray<float3x4> positions, NativeArray<float3x4> normals,float4x4 transform,  int resolution, JobHandle dependency
    );

	[BurstCompile(FloatPrecision.Standard, FloatMode.Fast, CompileSynchronously = true)]
	public struct Job<S > : IJobFor where S: struct, IShapeGenerator{
        /*
        [x0][x1][x2][x3]
        [y0][y1][y2][y3]
        [z0][z1][z2][z3]
        will be transpoed into 

        [x0][y0][z0]
        [x1][y1][z1]
        [x2][y2][z2]
        [x3][y3][z3]
        to get all x's in first column, y's in second and so on.. 
        this allows us to apply the same transform to 4 vectors at once, check Hashing.cs for the explanation under the 
        `TransformPositionVectorized` function.
        */
		[WriteOnly]
		NativeArray<float3x4> positions, normals;

        public float3x4 posTransform, normTransform;

		public float resolution, invResolution;
        
        // To keep the code before the generation of shape templates.
        public bool legacyPlaneOnlyPath;


        private float4x3 TransformPositionVectorized(float3x4 transform, float4x3 positions) =>  float4x3(
            // xx * x123 + xy * y123 + xz * z123 + xw (offset) 
			transform.c0.x * positions.c0 + transform.c1.x * positions.c1 + transform.c2.x * positions.c2 + transform.c3.x,
            // yx * x123 + yy * y123 + yz * z123 + yw (offset)
			transform.c0.y * positions.c0 + transform.c1.y * positions.c1 + transform.c2.y * positions.c2 + transform.c3.y,
            // zx * x123 + zy * y123 + zz * z123 + zw (offset)
			transform.c0.z * positions.c0 + transform.c1.z * positions.c1 + transform.c2.z * positions.c2 + transform.c3.z
		);


        // Contains a set of 4 points and 4 normals. (for vectorization purposes)
        public struct PackedPointInfo {
            public float4x3 pos, normals;
        }

        public static float3x4 NormalizeVectors(float3x4 v) => float3x4(normalize(v.c0),normalize(v.c1),normalize(v.c2), normalize(v.c3));

        // For each thread i compute position at index i in the array given a mathematical parametric shape (u = i//resolution , v = i % resolution mapped to [-0.5,0.5] interval)
		public void Execute (int i) {
            if (legacyPlaneOnlyPath){
            float4 vectorizedIdx = 4f * i + float4(0f, 1f, 2f, 3f);
            /*
            Unvectorized version
			float2 uv;
			uv.y = floor(invResolution * i + 0.00001f);
			uv.x = invResolution * (i - resolution * uv.y + 0.5f) - 0.5f;
			uv.y = invResolution * (uv.y + 0.5f) - 0.5f;
            */
            // uv is a float2 => packed into 4 = float4x2
			float4x2 uv;
			uv.c1 = floor(invResolution * vectorizedIdx + 0.00001f); 
            /*
            [0][floor(invResolution * i + 0.00001f)]
            [0][floor(invResolution * i + 0.00001f)]
            [0][floor(invResolution * i + 0.00001f)]
            [0][floor(invResolution * i + 0.00001f)]
            */
			uv.c0 = invResolution * (vectorizedIdx - resolution * uv.c1 + 0.5f) - 0.5f;
            /*
            [invResolution * (i - resolution * y + 0.5f) - 0.5f][floor(invResolution * i + 0.00001f)]
            [invResolution * (i - resolution * y + 0.5f) - 0.5f][floor(invResolution * i + 0.00001f)]
            [invResolution * (i - resolution * y + 0.5f) - 0.5f][floor(invResolution * i + 0.00001f)]
            [invResolution * (i - resolution * y + 0.5f) - 0.5f][floor(invResolution * i + 0.00001f)]
            */
			uv.c1 = invResolution * (uv.c1 + 0.5f) - 0.5f;
            /*
            [invResolution * (i - resolution * y0 + 0.5f) - 0.5f][invResolution * ( floor(invResolution * i0 + 0.00001f) + 0.5f) - 0.5f]
            [invResolution * (i - resolution * y1 + 0.5f) - 0.5f][invResolution * ( floor(invResolution * i1 + 0.00001f) + 0.5f) - 0.5f]
            [invResolution * (i - resolution * y2 + 0.5f) - 0.5f][invResolution * ( floor(invResolution * i2 + 0.00001f) + 0.5f) - 0.5f]
            [invResolution * (i - resolution * y3 + 0.5f) - 0.5f][invResolution * ( floor(invResolution * i3 + 0.00001f) + 0.5f) - 0.5f]
            */
			positions[i] = transpose(posTransform.TransformVectors(float4x3(uv.c0, 0f, uv.c1)));
            // Generates plane
            // Compute the local normal accounting for rotation of the game object transform
            normals[i] = NormalizeVectors(transpose(MathLib.TransformVectors(posTransform, float4x3(0f,1f,0f))));
            }
            else {
                ShapeGen.PackedPointInfo ppi = default(S).GeneratePoints(i, resolution, invResolution);
                positions[i] = transpose(posTransform.TransformVectors( ppi.pos));
                normals[i] = NormalizeVectors(transpose(normTransform.TransformVectors( ppi.normals, 0f)));
            }
		}

        /// <summary>
        /// Run the computation of the parametric shape
        /// </summary>
        public static JobHandle ScheduleParallel (
			NativeArray<float3x4> positions, NativeArray<float3x4> normals,float4x4 transform,  int resolution, JobHandle dependency
		) {
            float4x4 tim = transpose(inverse(transform));
			return new Job<S> {
				positions = positions,
				resolution = resolution,
				invResolution = 1f / resolution,
                normals = normals,
    			posTransform = transform.TruncateTo3x4(),
				normTransform = tim.TruncateTo3x4()
            }.ScheduleParallel(positions.Length, resolution, dependency);
		}

	}
        // Generates UV from index in the [0,1] range
        public static float4x2 mapToUV (int i, float resolution, float invResolution) {
            float4 vectorizedIdx = 4f * i + float4(0f, 1f, 2f, 3f);
			float4x2 uv;
			uv.c1 = floor(invResolution * vectorizedIdx + 0.00001f);  // index // resolution
			uv.c0 = invResolution * (vectorizedIdx - resolution * uv.c1 + 0.5f) ; // (index % resolution + 0.5)/resolution => u of center of "block"
			uv.c1 = invResolution * (uv.c1 + 0.5f); // (index // resolution + 0.5) / resolution  => v of center of "block" 
            return uv;
		}

        public struct PlaneGen: IShapeGenerator{

            // Generates a vector of 4 points
            public PackedPointInfo GeneratePoints(int i, float resolution, float invResolution){
                float4x2 uvs = mapToUV(i, resolution,invResolution);
                // Generates plane

                // Just transform the uv based on the object transform.
                // -0.5f because uv are generated in [0,1] space.
                float4x3 positions = float4x3(uvs.c0 - 0.5f, 0f, uvs.c1 - 0.5f);
                float4x3 normals = float4x3(0f,1f,0f);
                return new PackedPointInfo{
                    pos = positions,
                    normals = normals,
                };
            }
        }

        public struct SphereGen: IShapeGenerator{

            // Generates a vector of 4 points
            public PackedPointInfo GeneratePoints(int i, float resolution, float invResolution){
                float4x2 uvs = mapToUV(i, resolution,invResolution);
                // Generates plane

                float4 r = 0.5f;
                // sin here because between [0,PI] sin takes values from 0 to 1 to 0, which is what we need when you think about the vertical radius.
                float4 rVert = r * sin(PI * uvs.c1);


                // Just transform the uv based on the object transform.
                // -0.5f because uv are generated in [0,1] space.
                float4x3 positions = float4x3(
                     rVert * cos(2f * PI * uvs.c0), 
                    // we use cos here instead of cos because it will map [0,1] to the [-1,1] range when going from 0 to PI
                     r * cos(PI * uvs.c1),
                     rVert * sin(2f * PI * uvs.c0)
                    );
                // float4x3 normals = float4x3(0f,1f,0f);
                return new PackedPointInfo{
                    pos = positions,
                    normals = positions,
                };
            }
        }
        public struct TorusGen: IShapeGenerator{

            // Generates a vector of 4 points
            public PackedPointInfo GeneratePoints(int i, float resolution, float invResolution){
                float4x2 uvs = mapToUV(i, resolution,invResolution);
                // Generates plane

                float r1 = 0.375f;
                float r2 = 0.125f;
                float4 s = r1 + r2 * cos(2f * PI * uvs.c1);
                float4x3 positions;
                positions.c0 = s * sin(2f * PI * uvs.c0);
                positions.c1 = r2 * sin(2f * PI * uvs.c1);
                positions.c2 = s * cos(2f * PI * uvs.c0);
                // float4x3 normals = float4x3(0f,1f,0f);


                float4x3 normals;	
                normals = positions;
			    normals.c0 -= r1 * sin(2f * PI * uvs.c0);
			    normals.c2 -= r1 * cos(2f * PI * uvs.c0);
                return new PackedPointInfo{
                    pos = positions,
                    normals = positions,
                };
            }
        }

        // A sphere generated by normaluzing the positions of points in a a octahedron => points are more uniformly distributed.
        public struct OctaSphereGen: IShapeGenerator{

            // Generates a vector of 4 points
            public PackedPointInfo GeneratePoints(int i, float resolution, float invResolution){
                float4x2 uvs = mapToUV(i, resolution,invResolution);

                // start from a centered vertical plane
                float4x3 positions = float4x3(
                    uvs.c0 - 0.5f, // x
                    uvs.c1 - 0.5f, // y
                    0f
                    );
                // have 4 facets => z decreases together with x and y
                positions.c2 = /*center*/ 0.5f - abs(positions.c0) - abs(positions.c1);
                // reduce x and y the further z increases
                // ignore reduction if positive 
                float4 offset = max(-positions.c2, 0f);
                positions.c0 += select(-offset, offset, positions.c0 < 0f);
			    positions.c1 += select(-offset, offset, positions.c1 < 0f);

                // increase by vector size  (more at "center" of each face, none at vertices of octahedron)
			    float4 scale = 0.5f * rsqrt(
    				positions.c0 * positions.c0 +
    				positions.c1 * positions.c1 +
    				positions.c2 * positions.c2
			    );
			    positions.c0 *= scale;
			    positions.c1 *= scale;
			    positions.c2 *= scale;
                return new PackedPointInfo{
                    pos = positions,
                    normals = positions,
                };
            }
        }
}
