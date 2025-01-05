

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

    /// <summary>
    /// Updates the accumulator multiplying a prime with data and rotating the bits
    /// </summary>
    /// <param name="data"></param>
   	public SmallXXHashVectorized Eat (int4 data) =>
		RotateLeft(accumulator + (uint4)data * primeC, 17) * primeD;

    static uint4 RotateLeft (uint4 data, int steps) =>
    (data << steps) | (data >> 32 - steps);

    public static SmallXXHashVectorized Seed (int4 seed) => (uint4)seed + primeE;
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


	public static implicit operator SmallXXHashVectorized (SmallXXHash hash) =>
		new SmallXXHashVectorized(hash.accumulator);
}
