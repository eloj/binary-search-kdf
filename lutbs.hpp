/*
	Key-Derivation Functions
	See also: https://github.com/eloj/radix-sorting
*/
#include <algorithm>

// Helper template to return an unsigned T with the MSB set.
template<typename T>
constexpr typename std::make_unsigned<T>::type highbit(void) {
	return 1ULL << ((sizeof(T) << 3) - 1);
}

template<typename T, typename KT=T>
std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T,bool>, KT>
kdf(const T& value) {
	return value;
};

template<typename T, typename KT=T>
std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<T,bool>, KT>
kdf(const T& value) {
	return value ^ highbit<T>();
}

template<typename T, typename KT=uint32_t>
std::enable_if_t<std::is_same_v<T,float>, KT>
kdf(const T& value) {
	KT local;
	std::memcpy(&local, &value, sizeof(local));
	return local ^ (-(local >> 31UL) | (1UL << 31UL));
}

template<typename T, typename KT=uint64_t>
std::enable_if_t<std::is_same_v<T,double>, KT>
kdf(const T& value) {
	KT local;
	std::memcpy(&local, &value, sizeof(local));
	return local ^ (-(local >> 63UL) | (1UL << 63UL));
}
