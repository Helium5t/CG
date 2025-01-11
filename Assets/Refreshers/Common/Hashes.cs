

using static Unity.Mathematics.math;
using Unity.Mathematics;
using float4x4 = Unity.Mathematics.float4x4;
using quaternion = Unity.Mathematics.quaternion; 


// https://xxhash.com/ skipping some steps (2,3,4 from https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md)
// Reimplementation of SmallXXHash, but made so a burst job can vectorize the process.
public readonly struct SmallXXHashVectorized {

	// const uint primeA = 0b10011110001101110111100110110001;
	const uint primeB = 0b10000101111010111100101001110111;
	const uint primeC = 0b11000010101100101010111000111101;
	const uint primeD = 0b00100111110101001110101100101111;
	const uint primeE = 0b00010110010101100110011110110001;

    readonly uint4 accumulator;

	public SmallXXHashVectorized (uint4 seed) {
		this.accumulator = seed + primeE;
	}

    /*         OPERATORS         */
    public static implicit operator SmallXXHashVectorized (uint4 accumulator) =>
		new SmallXXHashVectorized(accumulator);

    /// <summary>
    /// implements final mixing steps of the accumulator
    /// </summary>
    /// <param name="hash"></param>
	public static implicit operator uint4 (SmallXXHashVectorized hash){
        uint4 avalanche = hash.accumulator;
		avalanche ^= avalanche >> 15;
		avalanche *= primeB;
		avalanche ^= avalanche >> 13;
		avalanche *= primeC;
		avalanche ^= avalanche >> 16;
		return avalanche;
    }

    public static SmallXXHashVectorized operator + (SmallXXHashVectorized h, int v)=>
        h.accumulator + (uint)v;

    /*     END OPERATORS      */

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits
    /// </summary>
    /// <param name="data"></param>
   	public SmallXXHashVectorized Eat (int4 data) =>
		RotateLeft(accumulator + (uint4)data * primeC, 17) * primeD;

    static uint4 RotateLeft (uint4 data, int steps) =>
    (data << steps) | (data >> 32 - steps);

    public static SmallXXHashVectorized Seed (int4 seed) => (uint4)seed + primeE;

    /// <summary>
    /// Shift generated number to the right rShift times and mask out the first count bits
    /// </summary>
    public uint4 GenerateBits(int bitCount, int rShift) => 
        ((uint4) this >> rShift) & (uint)(1 << bitCount) -1;


    public float4 GenerateBitsTo01(int bitCount, int rShift) => 
        (float4) GenerateBits(bitCount, rShift) * (1f / ((1<<bitCount) -1));


    // mask the first byte[0 ,255]
    public uint4 FirstByte => (uint4)this & 255;

    // Return the first byte mapped to [0.0,1.0] range
    public float4 MapATo01 => (float4) FirstByte*(1f/255f);

    // mask the second byte[0 ,255]
    public uint4 SecondByte => (uint4)this & 255;

    // Return the second byte mapped to [0.0,1.0] range
    public float4 MapBTo01 => (float4) SecondByte*(1f/255f);
    // mask the third byte[0 ,255]
    public uint4 ThirdByte => (uint4)this & 255;

    // Return the third byte mapped to [0.0,1.0] range
    public float4 MapCTo01 => (float4) ThirdByte*(1f/255f);

    // mask the fourth byte[0 ,255]
    public uint4 FourthByte => (uint4)this & 255;

    // Return the fourth byte mapped to [0.0,1.0] range
    public float4 MapDTo01 => (float4) FourthByte*(1f/255f);
}


// https://xxhash.com/ skipping some steps (2,3,4 from https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md)
public readonly struct SmallXXHash {

	const uint primeA = 0b10011110001101110111100110110001;
	const uint primeB = 0b10000101111010111100101001110111;
	const uint primeC = 0b11000010101100101010111000111101;
	const uint primeD = 0b00100111110101001110101100101111;
	const uint primeE = 0b00010110010101100110011110110001;

    readonly uint accumulator;

	public SmallXXHash (uint seed) {
		this.accumulator = seed + primeE;
	}

    /*        OPERATORS        */
    
    public static implicit operator SmallXXHash (uint accumulator) =>
		new SmallXXHash(accumulator);

    /// <summary>
    /// implements final mixing steps of the accumulator
    /// </summary>
    /// <param name="hash"></param>
	public static implicit operator uint (SmallXXHash hash){
        uint avalanche = hash.accumulator;
		avalanche ^= avalanche >> 15;
		avalanche *= primeB;
		avalanche ^= avalanche >> 13;
		avalanche *= primeC;
		avalanche ^= avalanche >> 16;
		return avalanche;
    }

	public static implicit operator SmallXXHashVectorized (SmallXXHash hash) =>
		new SmallXXHashVectorized(hash.accumulator);


    ////   END OPERATORS   ////

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits
    /// </summary>
    /// <param name="data"></param>
   	public SmallXXHash Eat (int data) =>
		RotateLeft(accumulator + (uint)data * primeC, 17) * primeD;

	

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits, uses different primes when using bytes vs using int
    /// </summary>
    /// <param name="data"></param>
   public SmallXXHash Eat (byte data) =>
		RotateLeft(accumulator + data * primeE, 11) * primeA;

    static uint RotateLeft (uint data, int steps) =>
    (data << steps) | (data >> 32 - steps);

    public static SmallXXHash Seed (int seed) => (uint)seed + primeE;
}
