#include <cstdio>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <immintrin.h>

#include <random>
#include <algorithm>
#include <limits>
#include <vector>
#include <array>
#include <iostream>
#include <bitset>

#include "lutbs.hpp"

template <typename T, typename GenFunc>
const std::vector<T> random_array(size_t len, GenFunc && gen) {
	std::vector<T> arr(len);

	std::generate_n(begin(arr), len, gen);

	return arr;
}

// Returns a mask with bits set for the bits that are relevant after applying the KDF.
// TODO: For signed integers we could do better; the relevant bits are all minus the min redundant leading sign bits (clrsb) + 1
template <
	typename T, typename KeyFunc = decltype(basic_kdfs::kdf<T>), typename KeyType=typename std::result_of_t<KeyFunc&&(T)>
>
auto relevant_bits(T *arr, size_t n, KeyFunc && kf = basic_kdfs::kdf) {
	KeyType acc_zeroes(0);
	KeyType acc_ones(~0);

	for (size_t i = 0 ; i < n ; ++i) {
		auto val = kf(arr[i]);
		acc_zeroes |= val;
		acc_ones   &= val;
	}

	// std::cout << "acc_zeroes = " << std::bitset<std::numeric_limits<T>::digits>(acc_zeroes) << "\n";
	// std::cout << "acc_ones   = " << std::bitset<std::numeric_limits<T>::digits>(acc_ones) << "\n";

	KeyType mask = ~(~acc_zeroes | acc_ones);

	return mask;
}

uint32_t bucket_no(uint32_t value, uint32_t shift, uint32_t lbits) {
	return ((value << shift) >> (32-lbits)) & ((1L << lbits)-1);
}

template <typename T, typename KeyFunc = decltype(basic_kdfs::kdf<T>)>
void demo(T *arr, size_t n, KeyFunc && kf = basic_kdfs::kdf) {

	auto mask = relevant_bits(arr, n, kf);
	auto max_compact = __builtin_popcount(~mask);

	std::cout << "mask   = " << std::bitset<std::numeric_limits<typeof(mask)>::digits>(mask) << "\n";

	// popcnt on the ~mask gives us the best 'compression' we can achieve.
	// For radix sorting this could provide an 'early out'. We only care about the thresholds 8, 16 and 24 since this saves us a pass each.
	uint32_t bitcast;
	uint32_t prefix_mask = 0;
	for (size_t i = 0 ; i < n ; ++i) {
		std::memcpy(&bitcast, arr + i, sizeof(bitcast)); // std::bit_cast<> not available
		auto value = kf(arr[i]);
		auto compact_value = _pext_u32(value, mask);

		printf("[%zu] kf(%08x) -> %08x -> %08x = bucket(%d)\n", i, bitcast, value, compact_value, bucket_no(compact_value, max_compact, 9));
		prefix_mask |= value;
	}
	// This is the 'naive' best shift we would get without the compaction.
	auto max_shift = __builtin_clz(prefix_mask);

	printf("Max compaction: %d of %d bits. Naive shift=%d\n", max_compact, std::numeric_limits<typeof(mask)>::digits, max_shift);

	// assert(max_shift == max_compact);
}

int main(int argc, char *argv[]) {
	std::array<uint32_t,7> ua = { 255, 42, 1, 0, 1, 42, (1L << 31) + 255 };
	std::array<int32_t,7> sa = { -255, -42, -1, 0, 1, 42, 255 };
	// std::array<float,11> fa = { 128.0f, 646464.0f, 1.987654321f, 0.0f, -0.0f, -0.5f, 0.5f, -128.0f, -INFINITY, NAN, INFINITY };
	std::array<float,6> fa = { 0.9f, 0.1f, 1.0f, 0.5f, 0.25f, 0.33333333f };

	demo(ua.data(), ua.size());
	demo(sa.data(), sa.size());
	demo(fa.data(), fa.size());

	return EXIT_SUCCESS;
}
