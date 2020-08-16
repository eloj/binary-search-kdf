/*
	WORK IN PROGRESS
	Experimenting with LUT'ed binary-search.

	See https://github.com/eloj/binary-search-kdf

	TODO:
		Write a lower_bound/binary_search that use the LUT to fetch the initial (maximum) range.
		Benchmark
*/
#include <cstdio>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <cassert>

#include "lutbs.hpp"

#include <random>
#include <limits>
#include <vector>
#include <array>

template<int LUTBits = 16, typename T, typename KeyFunc = decltype(kdf<T>)>
const std::vector<size_t> build_bs_lut(T *arr, size_t len, uint8_t extra_shift, KeyFunc && kf = kdf) {
	constexpr size_t lutlen = 1UL << LUTBits;
	std::vector<size_t> lut(lutlen + 1); // Add one as sentinel for now.

	uint32_t shift = ((sizeof(T) << 3) - LUTBits);

	printf("Building LUT<%d> with %zu entries, shift=%d, extra_shift=%d\n", LUTBits, lutlen, shift, extra_shift);

	shift -= extra_shift;

	size_t bucket = 0;
	size_t lo = 0;
	for (size_t i = 0 ; i < len - 1 ; ++i) {
		size_t next_bucket = kf(arr[i+1]) >> shift;
		printf("next bucket %zx\n", next_bucket);
		if (next_bucket != bucket) {
			assert(next_bucket > bucket && "Invalid input -- input not sorted or invalid kdf");
			lut[bucket] = lo;
			lo = i + 1;
			for (size_t j = bucket + 1 ; j < next_bucket ; ++j) {
				lut[j] = lo;
				printf("In-filled lut[%zx]=%zu\n", j, lut[j]);
			}

			bucket = next_bucket;
		}
	}

	for (size_t j = bucket ; j <= lutlen ; ++j) {
		lut[j] = len - 1;
		printf("Back-filled lut[%zx]=%zu\n", j, lut[j]);
	}

	return lut;
}

template<typename T, typename KeyFunc = decltype(kdf<T>)>
uint8_t array_lz(T *arr, size_t len, KeyFunc && kf = kdf) {
	auto key = kf(arr[0]);
	for (size_t i = 1 ; i < len ; ++i) {
		key |= kf(arr[i]);
	}

	if (key != 0 )
	{
		if constexpr (sizeof(T) <= 4)
			return __builtin_clz(key);
		else
			return __builtin_clzl(key);
	}

	return 0;
}

template <typename T, typename GenFunc>
const std::vector<T> random_array(size_t len, GenFunc && gen) {
	std::vector<T> arr(len);

	std::generate_n(begin(arr), len, gen);

	return arr;
}

int main(int argc, char *argv[]) {
	static const int N = 20;

	// std::random_device random_device();
	std::default_random_engine gen; // random_device()
	std::uniform_int_distribution<uint32_t> dist(0, 1UL << 17); // std::numeric_limits<uint32_t>::max());
	auto randgen = [&gen,&dist] { return dist(gen); };

	auto input = random_array<uint32_t>(N, randgen);

	// TODO: Use my own radix sort :-}
	std::sort(begin(input), end(input));

	// For unsigned integer types we could just check the maximum value (last), but it doesn't work in general due to KDF.
	uint32_t extra_shift = array_lz(input.data(), input.size());

	for (size_t i = 0 ; i < input.size() ; ++i) {
		auto val = input[i];
		printf("[%04zu] %08u (%08x -> kdf -> bucket %04x)\n", i, val, val, kdf(val) >> (32 - 5 - extra_shift));
	}

	auto lut = build_bs_lut<5>(input.data(), input.size(), extra_shift);
	// auto lut = build_bs_lut<5>(input.data(), input.size(), extra_shift, [](const uint32_t& value){ return value + 1; } );

	for (size_t i = 0 ; i < lut.size() - 1 ; ++i) {
		size_t keys = 0;
		if (lut[i] != lut[i+1])
			keys = lut[i+1] - lut[i];
		printf("LUT[%.*lx] = %04zu keys in [%zu..%zu)\n", 4, i, keys, lut[i], lut[i+1]);
	}

	uint32_t key = input[N-7];

	auto low = std::lower_bound(begin(input), end(input), key);

	printf("std::lower_bound for key %u is at position %zu\n", key, low - begin(input));

	// TODO ...
	// auto idx = lower_bound_lut(input.data(), input.size(), lut, key);
	// printf("our lower_bound for key %u is at position %zu\n", key, idx);

}
