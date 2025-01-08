using System;
using Unity.Mathematics;
using UnityEngine;
using static Unity.Mathematics.math;

public static partial class NoiseGen {
    [Serializable]
    public struct GenSettings{
        public int seed;

        /// <summary>
        /// Scales each coordinate to increase rate of change
        /// </summary>
        [Min(1)]
        public int frequency;

        /// <summary>
        /// How many levels of depth of the fractal. (i.e. iterations, frequency spaces etc...)
        /// At each octave the frequency increases and the range of the noise values decreases.
        /// </summary>
        [Range(1,6)]
        public int octaves;
        
		public static GenSettings Default => new GenSettings {
            frequency = 4,
            octaves = 1,
        };
    }
}