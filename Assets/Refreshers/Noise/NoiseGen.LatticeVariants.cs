using Unity.Mathematics;

using static Unity.Mathematics.math;

public static partial class NoiseGen {

    public interface ILattice{
        LatticeValuesVectorized GenerateLatticePoint(float4 coordinates, int frequency);
        
        /// <summary>
        /// Given a point it will return it's equivalent that is valid inside the lattice.
        /// </summary>
        int4 GetValidPoint(int4 points, int frequency);
    }

    /// <summary>
    /// Generates lattice points based on an infinite lattice scaled by frequency
    /// </summary>
    public struct OpenLattice : ILattice{
        public LatticeValuesVectorized GenerateLatticePoint(float4 coordinates, int frequency){ // frequency to be able to generate a tiling noise over the frequency
            // Scale the coordinate up by the frequency to increase rate of change across same value space
            coordinates *= frequency; 
            // get integer part
            float4 points = floor(coordinates);
            int4 ip = (int4) points;
            return new LatticeValuesVectorized{
                p0 = ip,
                p1 = ip+1,
                t = customSmoothing(coordinates - points), // map linear 0-1 range to Hermite interpolation 0-1
                g0 = coordinates - ip, // 0.01 to 0.99
                g1 = coordinates - ip - 1f, // -0.99 to -0.01
            };
        }

        public int4 GetValidPoint(int4 points, int frequency){
            return points;
        }
    }
    /// <summary>
    /// Generates lattice points that tiles with space size = frequency.
    /// </summary>
    public struct TilingLattice : ILattice{
        public LatticeValuesVectorized GenerateLatticePoint(float4 coordinates, int frequency){ // frequency to be able to generate a tiling noise over the frequency
            // Scale the coordinate up by the frequency to increase rate of change across same value space
            coordinates *= frequency; 
            // get integer part
            float4 points = floor(coordinates);
            int4 ip = (int4) points;
            LatticeValuesVectorized lvv =  new LatticeValuesVectorized{
                p0 = ip,
                t = customSmoothing(coordinates - points), // map linear 0-1 range to Hermite interpolation 0-1
                g0 = coordinates - ip, // 0.01 to 0.99
                g1 = coordinates - ip - 1f, // -0.99 to -0.01
            };
            lvv.p0 -= (int4)ceil(points/frequency) * frequency; // floating point division vectorizes, so do that and then cast
            lvv.p0 = select(lvv.p0, lvv.p0 + frequency, lvv.p0 < 0);
            lvv.p1 = lvv.p0 + 1;
            // Doing this avoid using the modulo, which does not vectorize.
            // Modulo requires division, which cannot be done as a vector operation.
            lvv.p1 = select(lvv.p1, 0, lvv.p1 == frequency); 
            return lvv;
        }
        public int4 GetValidPoint(int4 points, int frequency){
            return select(select(points, 0, points == frequency),frequency -1, points == -1);
        }
    }
}