using Unity.Mathematics;
using UnityEngine.Rendering;
using static Unity.Mathematics.math;

public static partial class NoiseGen {

    /// <summary>
    /// Given the Simplex Noise uses simplex shapes (line, triangle and dodecahedron)
    /// there is no point in passing a structure generator (open/tiling lattice) because 
    /// those shapes never align with a square/cube/hypercube grid. 
    /// Only exception is 1D (where the simplex is a line) but for conformity we will not use it.
    /// </summary>
	public struct Simplex1D<G> : INoiseGenerator where G : struct, IGradientEval {
		public float4 GenerateVectorizedNoise (float4x3 coords, SmallXXHashVectorized hash, int frequency) {
            coords *= frequency;
            int4 x0 = (int4) floor(coords.c0), x1 = x0 + 1 ; // Two points of the segment simplex.
            SmallXXHashVectorized noisePoint0 = hash.Eat(x0), noisePoint1 = hash.Eat(x1);
            // Simplex noise is the sum of the gradient evaluation of point P wrt the two points generating a segment simplex.
            float4 noise = Kernel(noisePoint0, x0, coords) + Kernel(noisePoint1, x1, coords);
			return default(G).EvaluateFinal(noise);
		}

        static float4 Kernel(SmallXXHashVectorized hash, float4 structurePoint, float4x3 coords) {
            float4 xGradient = coords.c0 - structurePoint; // the gradient is simply the fractional part for each position. in 1D at least.
            // f is essentially used to interpolate, so we need f to be a C2-continuous function so that at the extremes
            // of the interval [-1,1] the values of f' and f'' are 0,  (flat function with no acceleration at the extremes)
            //  (1 -x2)^3  satisfies the criteria.
            // f' =  -6x5 + 12x3 - 6x => (1)= -6 + 12 -6 =0 => (-1) = 6-12+6 = 0
            // f'' = -30x4 + 36x2 - 6 => (1)=(-1)=-30+36-6 = 0
            float4 g = 1-xGradient*xGradient;
            float4 f = g * g * g; 
            return f * default(G).Evaluate(hash, xGradient);
        }
	}

	public struct Simplex2D<G> :  INoiseGenerator where G : struct, IGradientEval {    /// <summary>
        /// To tile the xz plane with simplexes (equilateral triangles),
        /// we can start from the notion of a square skewed into a rhombus made up of two 
        /// equilateral triangles. Skeweing is just taking any coordinate and reducing by a skew factor.
        /// the skew factor changes with the coordinate though. So it's a function.
        /// So given the coordinates of the permiter of a square x,z
        /// xRhombus = x - s(x,z)
        /// zRhombus = z - s(x,z)
        /// The skewfactor is the same for both coordinates (consider the xz diagonal,
        /// a transformation along that line follows the rule x=z)
        /// It turns out that all s(x,z) functions always have a form of k * (z+v)
        /// So we can rewrtie s(x,z) = k(x+z)
        /// k = (3  - sqrt(3)) / 6      from squares to triangles
        /// k = (sqrt(3) - 1)  / 6      from triangles to squares
        /// </summary>
		public float4 GenerateVectorizedNoise (float4x3 coords, SmallXXHashVectorized hash, int frequency) {
			coords *= frequency* (1f / sqrt(3f)); // Scale by frequency and account for skewing.
            // Interpret coordinates as being in simplex space, so skew to get the euclidean space.
            // This is because when you think about where we should sample for the lattice (which is in euclidean)
            // we are coming from a "compressed" space where points are along the triangle-tiled space.
			float4 skew = (coords.c0 + coords.c2) * ((sqrt(3f) - 1f) / 2f);
			float4 
                sx = coords.c0 + skew, // euclidean coordinate for x
                sz = coords.c2 + skew; // euclidean coordinate for y
            int4   
                x0 = (int4) floor(sx) , x1 = x0+1, // Get lattice points for skewed coordinates
                z0 = (int4) floor(sz) , z1 = z0+1;
            bool4 inLowerTriangle = sx-x0 > sz-z0;
            int4
                xC = select(x0, x1, inLowerTriangle),
                zC = select(z1, z0, inLowerTriangle); // Choose x and z based on the side of the triangle (so fract(p))
            SmallXXHashVectorized
                h0 = hash.Eat(x0),
                h1 = hash.Eat(x1),
                hC = SmallXXHashVectorized.Select(h0, h1, inLowerTriangle);

            // Sum over all kernels computed with respct to the 4 different lattice points;
            float4 noise =
                    Kernel(h0.Eat(z0), x0, z0, coords) +
                    Kernel(h1.Eat(z1), x1, z1, coords) +
                    Kernel(h0.Eat(z1), x0, z1, coords) +
                    Kernel(h1.Eat(z0), x1, z0, coords) ;
                    // Kernel(hC.Eat(zC), xC, zC, coords) ; // Third triangle point based on fract(p.x) > fract(p.z)
            return  default(G).EvaluateFinal(noise);
		}

    
        static float4 Kernel(SmallXXHashVectorized hash, float4 latticePx, float4 latticePz, float4x3 coords) {
            // Turn coordinates back to simplex space. 
            float4 skewFactor = (latticePx + latticePz) * ((3f - sqrt(3f))/ 6f); 

            // This introduces an issue where we will compute the kernel
            // for values beyond the influence of the lattice point,
            // due to euclidean distances between lattice points being
            // greater compared to simplex distances. 
            float4 dx = coords.c0 - (latticePx - skewFactor);
            float4 dz = coords.c2 - (latticePz - skewFactor);
            float4 dist = dx * dx + dz * dz;

            // We'll fix the computation issue by reducing the descent by 0.5 
            // as it's the maximum distance we can reach before going inside
            // another triangle. (height of an equilateral triangle of side sqrt(2/3) )
            float4 g = 0.5f - dist;

            // Scale by 8 because g is now in [0, 0.5] => g^3 is in [0, 1/8]
            float4 f =  8f * g * g * g;
            return max(f ,0f) * default(G).Evaluate(hash, dx, dz);
        }
	}

	public struct Simplex3D<G> : INoiseGenerator where G : struct, IGradientEval {
        /// <summary>
        /// We collapse the lattice of cubes into a lattice of isorombohedrons.
        /// </summary>
		public float4 GenerateVectorizedNoise (float4x3 coords, SmallXXHashVectorized hash, int frequency) {
            coords *= frequency * 0.6f; // just to have a comparbale scale.

            // k = 1/3 to go from simplex to euclidean.
            float4 skewFactor = (coords.c0 + coords.c1 + coords.c2) * (1f/3f);
            float4 
                sx =  coords.c0 + skewFactor,
                sy =  coords.c1 + skewFactor,
                sz =  coords.c2 + skewFactor;
            
            int4 
                x0 = (int4) floor(sx), x1 = x0 + 1,
                y0 = (int4) floor(sy), y1 = y0 + 1,
                z0 = (int4) floor(sz), z1 = z0 + 1;
            
            bool4 // the cube is partitioned in 2 dodecahedron per face => 12 choices
                YlowerX = sx - x0 > sy - y0,
                ZlowerX = sx - x0 > sz - z0,
                ZlowerY = sy - y0 > sz - z0;
            
            bool4  // Find dodecahedron section
                xA = YlowerX & ZlowerX, 
				xB = YlowerX | (ZlowerX & ZlowerY), 
				yA = !YlowerX & ZlowerY,
				yB = !YlowerX | (ZlowerX & ZlowerY),
				zA = (YlowerX & !ZlowerX) | (!YlowerX & !ZlowerY),
				zB = !(ZlowerX & ZlowerY);
            
            int4  // select dodecahedron coordinates for lattice
				xCA = select(x0, x1, xA), 
				xCB = select(x0, x1, xB), 
				yCA = select(y0, y1, yA), 
				yCB = select(y0, y1, yB), 
				zCA = select(z0, z1, zA), 
				zCB = select(z0, z1, zB);


            
            SmallXXHashVectorized h0 = hash.Eat(x0), h1 = hash.Eat(x1),
                hA = SmallXXHashVectorized.Select(h0,h1,xA),
                hB = SmallXXHashVectorized.Select(h0,h1, xB);

            return default(G).EvaluateFinal(
                Kernel(h0.Eat(y0).Eat(z0), x0, y0, z0, coords)+
                Kernel(h1.Eat(y1).Eat(z1), x1, y1, z1, coords)+
                Kernel(hA.Eat(yCA).Eat(zCA), xCA, yCA, zCA, coords) +
				Kernel(hB.Eat(yCB).Eat(zCB), xCB, yCB, zCB, coords)
            );
		}

        public float4 Kernel(SmallXXHashVectorized h, float4 latticePx, float4 latticePy, float4 latticePz, float4x3 coords){
            // k = 1/6 to go from euclidean to simplex space.
            float4 skewFactor = (latticePx + latticePy + latticePz) * (1f/6f); 
            float4
                dx = coords.c0 - latticePx + skewFactor,
                dy = coords.c1 - latticePy + skewFactor,
                dz = coords.c2 - latticePz + skewFactor;
            
            float4 g = 0.5f - dx*dx - dy*dy - dz*dz;
            float4 f = 8 * g * g * g;
            return max(0f, f) * default(G).Evaluate(h, dx, dy, dz);
        }
	}
}