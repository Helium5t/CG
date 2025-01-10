using Unity.Mathematics;

using static Unity.Mathematics.math;
using static Noise;
using System.Security.Cryptography.X509Certificates;
using UnityEngine.InputSystem.Android;
using UnityEngine;
using UnityEditor.Overlays;


public static partial class NoiseGen {


    /// <summary>
    /// Operates a vectorized select and returns the minimum vector possible from the two.
    /// </summary>
    /// <returns>The shortest distance for each of the 4 values.e.g.: <br /><c >
    ///     shortest  = [2.0,1.0,0.5,0.1]
    ///     distances = [0.1,0.5,1.0,2.0]<br />  
    ///     returned  = [0.1,0.5,0.5,0.1]
    ///     </c>
    /// </returns>
    static float4 UpdateShortestDistance(float4 shortest, float4 distances){
        return select(shortest, distances, distances < shortest);
    }

    /// <summary>
    /// Generates Voronoi noise by hashing the coordinates and 
    /// generating a point inside the [0.01,0.99] range (fract of hash).
    /// Distance is computed as distance from that point
    /// </summary>
    public struct Voronoi1D<L> : INoiseGenerator where L: struct, ILattice{
        


        public float4 GenerateVectorizedNoise(float4x3 positions, SmallXXHashVectorized hash, int frequency)
        {
            LatticeValuesVectorized x = default(L).GenerateLatticePoint(positions.c0, frequency);
            float4 shortestDist = 2f; // 2 is the max value as range value is [0,1] so max dist is double that.
            for (int pi = -1; pi <= 1; pi++ ){

                SmallXXHashVectorized h = hash.Eat(default(L).GetValidPoint(x.p0 + pi, frequency)); // x.p0 = int(coordinates) from shape generation.
                shortestDist = UpdateShortestDistance(shortestDist, abs(h.MapATo01 + pi/*because this point is pi away from evaluation*/ - x.g0));
            }
            return shortestDist;
        }
    }
    public struct Voronoi2D<L> : INoiseGenerator where L: struct, ILattice{
        

        public float4 GenerateVectorizedNoise(float4x3 positions, SmallXXHashVectorized hash, int frequency)
        {
            L lat = default(L);
            LatticeValuesVectorized x = lat.GenerateLatticePoint(positions.c0, frequency),
                                    z = lat.GenerateLatticePoint(positions.c2, frequency);
            float4 shortestDist = 8f; // max distance can at most be 1 sqrt(2) * 2 given the lattice structure
            for (int xi = -1 ; xi <= 1; xi ++){
                SmallXXHashVectorized hx = hash.Eat(lat.GetValidPoint(x.p0 + xi, frequency)); 
                for (int zi = -1; zi <=1; zi ++){
                    SmallXXHashVectorized h = hx.Eat(lat.GetValidPoint(z.p0 + zi, frequency));
                    
                    float4 dx1 = h.MapATo01 + xi - x.g0, dz1 = h.MapBTo01 + zi - z.g0,
                    dx2 = h.MapCTo01 + xi - x.g0, dz2 = h.MapDTo01 + zi - z.g0;
                    shortestDist = UpdateShortestDistance(shortestDist,dx1 * dx1 + dz1 * dz1);
                    shortestDist = UpdateShortestDistance(shortestDist,dx2 * dx2 + dz2 * dz2);
                }
            }
            return min(sqrt(shortestDist),1f);
        }
    }
    public struct Voronoi3D<L> : INoiseGenerator where L: struct, ILattice{
        

        public float4 GenerateVectorizedNoise(float4x3 positions, SmallXXHashVectorized hash, int frequency)
        {   
            L lat = default(L);
            LatticeValuesVectorized x = lat.GenerateLatticePoint(positions.c0, frequency),
                                    y = lat.GenerateLatticePoint(positions.c1, frequency),
                                    z = lat.GenerateLatticePoint(positions.c2, frequency);
            float4 shortestDist = 12f; // max distance can at most be sqrt(3) * 2 given the lattice structure
            for (int xi = -1 ; xi <= 1; xi ++){
                SmallXXHashVectorized hx = hash.Eat(lat.GetValidPoint(x.p0 + xi, frequency));
                for (int zi = -1; zi <=1; zi ++){
                    SmallXXHashVectorized hz = hx.Eat(lat.GetValidPoint(z.p0 + zi , frequency));
                    for(int yi = -1; yi <= 1; yi ++){
                        SmallXXHashVectorized h = hz.Eat(lat.GetValidPoint(y.p0 + yi , frequency));
                        float4 dx = h.MapATo01 + xi - x.g0, dz = h.MapBTo01 + zi - z.g0, dy = h.MapCTo01 + yi - y.g0;
                        shortestDist = UpdateShortestDistance(shortestDist,dx * dx + dz * dz + dy * dy);
                    }
                }
            }
            return min(sqrt(shortestDist),1f);
        }
    }
}