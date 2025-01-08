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

        /// <summary>
        /// Allows to "skip" frequency spaces. 
        /// Usually you double the frequency with each octave.
        /// Lacunarity defines the iteration multiplies 
        ///
        /// e.g.    lacunarity = 2 => f *= 2 at each iteration : 1,2,4...
        ///         lacunarity = 3 => f *= 3 at each iteration : 1,3,9...
        /// </summary>
        [Range(2,4)]
        public int lacunarity;

        /// <summary>
        /// The same as lacunarity, but applied to amplitude.
        /// Essentially how long the "signal" lives for, if you think about
        /// noise as the Moving Average of the last x signals where x = octaves.
        /// By default persistence is 0.5, so at each octave the signal halves its
        /// strength.
        /// </summary>
        [Range(0f,1f)]
        public float persistence;

        
		public static GenSettings Default => new GenSettings {
            frequency = 4,
            octaves = 1,
            lacunarity = 2,
            persistence = 0.5f,
        };
    }
}