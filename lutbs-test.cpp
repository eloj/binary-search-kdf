/*
	WORK IN PROGRESS
	Experimenting with LUT'ed binary-search.

	See https://github.com/eloj/binary-search-kdf

	TODO:
		Work out issues with signed/float types.
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
const std::vector<size_t> build_bs_lut(const T *arr, size_t len, uint8_t extra_shift = 0, KeyFunc && kf = kdf) {
	constexpr size_t lutlen = 1UL << LUTBits;
	const uint32_t shift = ((sizeof(T) << 3) - LUTBits) - extra_shift;

	std::vector<size_t> lut(lutlen + 1); // Add one as sentinel for now.

	printf("Building LUT<%d> with %zu entries, shift=%d, extra_shift=%d from %zu entries.\n", LUTBits, lutlen, shift, extra_shift, len);

	size_t bucket = 0;
	size_t lo = 0;
	for (size_t i = 0 ; i < (len - 1) ; ++i) {
		// printf("Probing arr[%zu] => %d -> %d\n", i+1, arr[i + 1], kf(arr[i+1]));
		size_t next_bucket = kf(arr[i+1]) >> shift;
		assert(next_bucket < lutlen);

		if (next_bucket != bucket) {
			assert(next_bucket > bucket && "Invalid input -- input not sorted or invalid kdf");
			lut[bucket] = lo;
			lo = i + 1;
			for (size_t j = bucket + 1 ; j < next_bucket ; ++j) {
				lut[j] = lo;
				// printf("In-filled lut[%zx]=%zu\n", j, lut[j]);
			}

			bucket = next_bucket;
		}
	}

	for (size_t j = bucket ; j <= lutlen ; ++j) {
		lut[j] = lo;
		lo = len;
		// printf("Back-filled lut[%zx]=%zu\n", j, lut[j]);
	}

	return lut;
}

// We'd should maybe consider packaging the lut in an opaque type with LUTBits and extra_shift, maybe a ref to the KeyFunc?
template<typename T, typename KeyFunc = decltype(kdf<T>)>
size_t lower_bound_lut(const T* arr, size_t len, const std::vector<size_t>& lut, T key, uint8_t extra_shift = 0, KeyFunc && kf = kdf) {
	assert(lut.size() > 0);
	assert(lut.size() & 1);

	const int LUTBits = __builtin_ctz(lut.size() & (lut.size()-1)); // Mask out lowest bit due to sentinel.
	const uint32_t shift = ((sizeof(T) << 3) - LUTBits) - extra_shift;

	printf("lut size=%zu, derived lut bits: %d, lut shift=%d\n", lut.size(), LUTBits, shift);

	auto keyval = kf(key);
	size_t bucket = keyval >> shift;
	size_t l = lut[bucket];
	size_t r = lut[bucket+1];

	assert(l <= r);

	printf("key=%x, kf(key)=%x, bucket=%zu, searching range [%zu, %zu)\n", key, keyval, bucket, l, r);


#if 0
	// TODO: Linear scan for small ranges (req. benchmarks)
	// TODO: arr[l] either matches the key -- scan left to find start of range, or is smaller than key -- scan right.
	// Could maybe use LUT data as heuristic for when to switch to linear scan?
	size_t linear_threshold = 10;
	if (r - l <= linear_threshold) {
		printf("Linear scan from %zu to %zu\n", l, r);
		for (; l < r ; ++l) {
			if (arr[l] == key)
				return l;
		}
		return len;
	}
#endif

	size_t n_probes = 0;
	// Binary search for the left-most element.
	while (l < r) {
		++n_probes;
		size_t m = l + ((r-l) >> 1);
		// printf("Probing index %zu\n", m);
		assert(m < r);
		if (arr[m] < key)
			l = m + 1;
		else
			r = m;
	}

	printf("Num probes: %zu\n", n_probes);

	return arr[l] == key ? l : len; // NOTE: Returns OOB if key not found, similar to std::lower_bound
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

template<typename T, typename KeyFunc = decltype(kdf<T>)>
void std_sort_kdf(T *arr, size_t len, KeyFunc && kf = kdf) {
	std::sort(arr, arr + len, [&kf](const T a, const T b) { return kf(a) < kf(b); });
}

template <typename T, typename GenFunc>
const std::vector<T> random_array(size_t len, GenFunc && gen) {
	std::vector<T> arr(len);

	std::generate_n(begin(arr), len, gen);

	return arr;
}

int main(int argc, char *argv[]) {
	typedef uint32_t ArrT;

	const int N = argc > 1 ? uint32_t(atoi(argv[1])) : 200;
	constexpr int LBITS = 5;

	// std::random_device random_device();
	std::default_random_engine gen; // random_device()
	std::uniform_int_distribution<ArrT> dist(0, (1UL << 17)-1); // std::numeric_limits<uint32_t>::max());
	auto randgen = [&gen,&dist] { return dist(gen); };

	auto input = random_array<ArrT>(N, randgen);

	std_sort_kdf(input.data(), input.size());

	// For unsigned integer types we could just check the maximum value (last), but it doesn't work in general due to KDF.
	// In a production system with a radix sort, this could be extracted in the histogramming step to save the scan.
	uint32_t extra_shift = array_lz(input.data(), input.size());
	// extra_shift = 0;

	for (size_t i = input.size() - 10 ; i < input.size() ; ++i) {
		auto val = input[i];
		printf("[%04zu] %08u (%08x -> kdf -> bucket %04x)\n", i, val, val, kdf(val) >> (sizeof(val)*8 - LBITS - extra_shift));
	}

	auto lut = build_bs_lut<LBITS>(input.data(), input.size(), extra_shift);
	// auto lut = build_bs_lut<LBITS>(input.data(), input.size(), extra_shift, [](const uint32_t& value){ return value + 1; } );

	for (size_t i = 0 ; i < lut.size() - 1 ; ++i) {
		size_t keys = 0;
		if (lut[i] != lut[i+1])
			keys = lut[i+1] - lut[i];
		if (keys > 0)
			printf("LUT[%.*lx] = %04zu keys in [%zu..%zu)\n", 4, i, keys, lut[i], lut[i+1]);
	}

	printf("LUT is %zu bytes + sentinel.\n", sizeof(size_t)*(lut.size()-1));

	ArrT key = argc > 2 ? atoi(argv[2]) : input[N-7];

	auto low = std::lower_bound(begin(input), end(input), key);

	printf("std::lower_bound for key %u is at position %zu\n", key, low - begin(input));

	auto idx = lower_bound_lut(input.data(), input.size(), lut, key, extra_shift);
	printf("our lower_bound for key %u is at position %zu\n", key, idx);
	if (idx > input.size() - 1) {
		printf("KEY NOT FOUND.\n");
	} else if (idx != size_t(low - begin(input))) {
		printf("KEY MISMATCH ERROR!\n");
	}

}
