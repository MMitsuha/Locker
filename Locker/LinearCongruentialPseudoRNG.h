#pragma once

/*////////////////////////////////////////////////////////////////////////////////*\
Simple implementation of a Linear Congruential Pseudo-Random Number Generator (PRNG)
https://en.wikipedia.org/wiki/Linear_congruential_generator
https://www.cplusplus.com/reference/random/minstd_rand/
\*////////////////////////////////////////////////////////////////////////////////*/

// modulus (2^31 - 1) or 0x7FFFFFFF in hex
constexpr uint32_t m{ 2147483647 };
static_assert(m > 0, "modulus is not greater than 0!");

// multiplier
constexpr uint32_t a{ 48271 };
static_assert(0 < a && a < m, "multiplier is not between 0 and modulus!");

// constant / increment
constexpr uint32_t c{ 0 };
static_assert(0 <= c && c < m, "increment is not between 0 and modulus!");

// Defining the seed based on compile time in seconds since 00:00:00
constexpr uint32_t seed = ((__TIME__[7] - '0') * 1 + (__TIME__[6] - '0') * 10
	+ (__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600
	+ (__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000);
static_assert(0 <= seed && seed < m, "seed is not between 0 and modulus!");

// This ensures that RNG() is evaluated at compile time! (Keyword: Template Metaprogramming)
template <uint32_t Const>
struct MakeConst
{
	enum { Value = Const };
};

constexpr uint32_t RecursiveRNG(uint32_t num)
{
	return (c + a * ((num > 0) ? RecursiveRNG(num - 1) : seed)) & m;
}

// Outputs random number as constexpr
#define LCG_RANDOM() (MakeConst<RecursiveRNG(__COUNTER__ + 1)>::Value)
// Outputs a positive number in the given range
#define LCG_RANDOM_IN_RANGE(min, max) (min + (LCG_RANDOM() & m) % (max - min + 1))
// This acts as a bool. Could have use a range of 0 and 1, but this might have introduced more bias
#define LCG_BOOL (LCG_RANDOM_IN_RANGE(0, 100) % 2)
