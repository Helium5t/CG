using Unity.Mathematics;

using static Unity.Mathematics.math;
using static Noise;
using System.Security.Cryptography.X509Certificates;
using UnityEngine.InputSystem.Android;
using UnityEngine;
using UnityEditor.Overlays;
using UnityEditor.Experimental.GraphView;
using System;


public static partial class NoiseGen {
    public interface IDistanceFunction {
        float4 GetDistance(float4 x);
        float4 GetDistance(float4 x,float4 y);
        float4 GetDistance(float4 x,float4 y,float4 z);

		float4x2 Finalize1D (float4x2 shortest);

		float4x2 Finalize2D (float4x2 shortest);

		float4x2 Finalize3D (float4x2 shortest);
    }

    public struct ManhattanDistance : IDistanceFunction{
        public float4 GetDistance(float4 x) => abs(x);
        public float4 GetDistance(float4 x,float4 y) => abs(x) + abs(y);
        public float4 GetDistance(float4 x,float4 y,float4 z) => abs(x) + abs(y) + abs(z);
		public float4x2 Finalize1D (float4x2 shortest) =>shortest;
		public float4x2 Finalize2D (float4x2 shortest)=> Clamp(shortest);//RestrictRange(shortest, 2f);
        public float4x2 Finalize3D (float4x2 shortest)=> Clamp(shortest); //RestrictRange(shortest, 3f);

        private float4x2 Clamp(float4x2 shortest){
            return float4x2(
                min(shortest.c0, 1f),
                min(shortest.c1, 1f)
            );
        }
        /// <summary>
        /// Given we sum each dimension, we need to
        /// scale the range based on the maximum value to keep a [0,1] range
        /// </summary>
        private float4x2 RestrictRange(float4x2 shortest, float range){
            shortest.c0 /= range;
            shortest.c1 /= range;
            return shortest;
        }
    }

    /// <summary>
    /// When using Euclidean Distance, Voronoi noise is the same as Worley noise.
    /// Worley noise is a Voronoi noise that uses Eucliden distance as noise value.
    /// </summary>
    public struct EuclideanDistance : IDistanceFunction{
        public float4 GetDistance(float4 x) => abs(x);
        public float4 GetDistance(float4 x,float4 y) => x*x + y*y;
        public float4 GetDistance(float4 x,float4 y,float4 z) => x*x + y*y + z*z;
		public float4x2 Finalize1D (float4x2 shortest) =>shortest;
		public float4x2 Finalize2D (float4x2 shortest)=> ClampDistances(shortest);
        public float4x2 Finalize3D (float4x2 shortest)=> ClampDistances(shortest);
    }

    /// <summary>
    /// Also known as Chebyshev Noise. It is Voronoi noise that uses Chessboard distance.
    /// So it uses the great distance along the three dimension as metric for noise values.
    /// </summary>
    public struct ChessboardDistance : IDistanceFunction{
        public float4 GetDistance(float4 x) => abs(x);
        public float4 GetDistance(float4 x,float4 y) => max(abs(x) ,abs(y));
        public float4 GetDistance(float4 x,float4 y,float4 z) => max(abs(x),max(abs(y),abs(z)));
		public float4x2 Finalize1D (float4x2 shortest) =>shortest;
		public float4x2 Finalize2D (float4x2 shortest)=> shortest;
        public float4x2 Finalize3D (float4x2 shortest)=> shortest;
    }

    /// <summary>
    /// Interface to feed a different distance to Voronoi noise
    /// </summary>
    public interface IDistanceSelectionFunction{
        float4 EvaluateDistances(float4x2 shortest);
    }

    public struct F1 : IDistanceSelectionFunction {

		public float4 EvaluateDistances (float4x2 distances) => distances.c0;
	}

	public struct F2 : IDistanceSelectionFunction {

		public float4 EvaluateDistances (float4x2 distances) => distances.c1;
	}

	public struct F2MinusF1 : IDistanceSelectionFunction {

		public float4 EvaluateDistances (float4x2 distances) => distances.c1-distances.c0;
	}

    /// <summary>
    /// Operates a vectorized select and returns the minimum vector possible from the two.
    /// </summary>
    /// <returns>The shortest distance for each of the 4 values.e.g.: <br /><c >
    ///     shortest  = [2.0,1.0,0.5,0.1]
    ///     distances = [0.1,0.5,1.0,2.0]<br />  
    ///     returned  = [0.1,0.5,0.5,0.1]
    ///     </c>
    /// </returns>
    static float4x2 UpdateShortestDistances(float4x2 shortest, float4 distances){
        bool4 hasNewShortest = distances < shortest.c0; // Check if we have one distance that is the shortest.
        shortest.c1 = select(
            select(shortest.c1, distances, shortest.c1 > distances), // Check if we have one distance that is shorter than second shortest. 
            shortest.c0,
            hasNewShortest);
        shortest.c0 = select(shortest.c0, distances, hasNewShortest);

        return shortest;
        // return select(shortest, distances, distances < shortest);
    }

    static float4x2 ClampDistances(float4x2 shortest){
        return float4x2(
            min(sqrt(shortest.c0),1f),
            min(sqrt(shortest.c1),1f)
        );
    }

    /// <summary>
    /// Generates Voronoi noise by hashing the coordinates and 
    /// generating a point inside the [0.01,0.99] range (fract of hash).
    /// Distance is computed as distance from that point
    /// </summary>
    public struct Voronoi1D<L, D, F> : INoiseGenerator where L: struct, ILattice where F: struct, IDistanceSelectionFunction where D: struct, IDistanceFunction{
        


        public float4 GenerateVectorizedNoise(float4x3 positions, SmallXXHashVectorized hash, int frequency)
        {
            D df = default(D);
            LatticeValuesVectorized x = default(L).GenerateLatticePoint(positions.c0, frequency);
            float4x2 shortestDist = 2f; // 2 is the max value as range value is [0,1] so max dist is double that.
            for (int pi = -1; pi <= 1; pi++ ){

                SmallXXHashVectorized h = hash.Eat(default(L).GetValidPoint(x.p0 + pi, frequency)); // x.p0 = int(coordinates) from shape generation.
                shortestDist = UpdateShortestDistances(shortestDist,df.GetDistance( h.MapATo01 + pi/*because this point is pi away from evaluation*/ - x.g0));
            }
            return default(F).EvaluateDistances(df.Finalize1D(shortestDist));
        }
    }
    public struct Voronoi2D<L, D, F> : INoiseGenerator where L: struct, ILattice where F: struct, IDistanceSelectionFunction where D:struct, IDistanceFunction{
        

        public float4 GenerateVectorizedNoise(float4x3 positions, SmallXXHashVectorized hash, int frequency)
        {
            D df = default(D);
            L lat = default(L);
            LatticeValuesVectorized x = lat.GenerateLatticePoint(positions.c0, frequency),
                                    z = lat.GenerateLatticePoint(positions.c2, frequency);
            float4x2 shortestDist = 2f; // max distance can at most be 1 sqrt(2) * 2 given the lattice structure
            for (int xi = -1 ; xi <= 1; xi ++){
                SmallXXHashVectorized hx = hash.Eat(lat.GetValidPoint(x.p0 + xi, frequency)); 
                for (int zi = -1; zi <=1; zi ++){
                    SmallXXHashVectorized h = hx.Eat(lat.GetValidPoint(z.p0 + zi, frequency));
                    
                    // Here we generate two points per lattice, because this will reduce the likelihood of 
                    // distances >1, as it would cause us to have to have a lookup for up to
                    // two points ahead. (think of a circle of sqrt(2) centered in one of the corners)
                    float4 dx1 = h.MapATo01 + xi - x.g0, dz1 = h.MapBTo01 + zi - z.g0,
                    dx2 = h.MapCTo01 + xi - x.g0, dz2 = h.MapDTo01 + zi - z.g0;
                    shortestDist = UpdateShortestDistances(shortestDist, df.GetDistance(dx1,dz1));
                    shortestDist = UpdateShortestDistances(shortestDist, df.GetDistance(dx2,dz2));
                }
            }
            return default(F).EvaluateDistances(df.Finalize2D(shortestDist));
        }
    }
    public struct Voronoi3D<L, D, F> : INoiseGenerator where L: struct, ILattice where F: struct, IDistanceSelectionFunction where D: struct,IDistanceFunction{
        

        public float4 GenerateVectorizedNoise(float4x3 positions, SmallXXHashVectorized hash, int frequency)
        {   
            D df = default(D);
            L lat = default(L);
            LatticeValuesVectorized x = lat.GenerateLatticePoint(positions.c0, frequency),
                                    y = lat.GenerateLatticePoint(positions.c1, frequency),
                                    z = lat.GenerateLatticePoint(positions.c2, frequency);
            float4x2 shortestDist = 2f; // max distance can at most be sqrt(3) * 2 given the lattice structure
            for (int xi = -1 ; xi <= 1; xi ++){
                SmallXXHashVectorized hx = hash.Eat(lat.GetValidPoint(x.p0 + xi, frequency));
                for (int zi = -1; zi <=1; zi ++){
                    SmallXXHashVectorized hz = hx.Eat(lat.GetValidPoint(z.p0 + zi , frequency));
                    for(int yi = -1; yi <= 1; yi ++){
                        SmallXXHashVectorized h = hz.Eat(lat.GetValidPoint(y.p0 + yi , frequency));
                        float4 dx1 = h.GenerateBitsTo01(5,0) + xi - x.g0, dz1 = h.GenerateBitsTo01(5,10) + zi - z.g0, dy1 = h.GenerateBitsTo01(5,5) + yi - y.g0;
                        float4 dx2 = h.GenerateBitsTo01(5,15) + xi - x.g0, dz2 = h.GenerateBitsTo01(5,25) + zi - z.g0, dy2 = h.GenerateBitsTo01(5,20) + yi - y.g0;
                        shortestDist = UpdateShortestDistances(shortestDist,df.GetDistance(dx1, dz1, dy1) );
                        shortestDist = UpdateShortestDistances(shortestDist,df.GetDistance(dx2, dz2, dy2) );
                    }
                }
            }
            return default(F).EvaluateDistances(df.Finalize3D(shortestDist));
        }
    }
}