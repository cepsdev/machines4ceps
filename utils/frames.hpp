#pragma once 

#include <algorithm>

template<typename ...> class frame_layout;

template<int c> class bw{
public:
	static constexpr int w = c;
};

template<> class frame_layout<> {
	
};

template<typename T , typename... Ts> class frame_layout<T, Ts...>: public frame_layout<Ts...> {
};

template<int c, typename T, typename... Ts> inline int get_ofs(frame_layout<T, Ts...> const &  = frame_layout<T, Ts...>{}) {
	return c == 0 ? 0 : std::abs(T::w) + get_ofs<c - 1,Ts...>();
}

template<int c> inline int get_ofs(frame_layout<> const &  = frame_layout<>{}) {
	return 0;
}

template<int c, typename T, typename... Ts>  int get_bit_width(frame_layout<T, Ts...> const & = frame_layout<T, Ts...>{}) {
	return  c == 0 ? T::w : get_bit_width<c - 1,Ts...>();
}

template<int c>  int get_bit_width(frame_layout<> const & = frame_layout<>{}) {
	return 0;
}

template<typename T, int c, typename R = std::uint64_t> R get_value(char* mem) {
	auto bw = get_bit_width<c>(T{});
	if (bw == 0) return 0;
	const std::size_t byte_ofs = get_ofs<c>(T{}) / 8 ;	
	const unsigned short start_bit = get_ofs<c>(T{}) % 8;
	const int bytes_to_fetch = std::min<std::size_t>( (bw + start_bit + 7) / 8, sizeof R );
	R r{};
	memcpy(&r,mem + byte_ofs, bytes_to_fetch);
	r >>= start_bit;
	std::uint64_t bwr = bw % 8;
	if (bwr == 1) bwr = 1;
	else if (bwr == 2) bwr = 3;
	else if (bwr == 3) bwr = 7;
	else if (bwr == 4) bwr = 15;
	else if (bwr == 5) bwr = 31;
	else if (bwr == 6) bwr = 63;
	else if (bwr == 7) bwr = 127;
	else bwr = 0xFF;
	if (bw <= 8) {
		r &= 0xFFUL & bwr;
	} else if (bw <= 16) {
		r &= (bwr << 8) + 0xFF;
	} else if (bw <= 24) {
		r &= (bwr << 16) + 0xFFFFUL;
	} else if (bw <= 32) {
		r &= (bwr << 24) + 0xFFFFFFUL;
	} else if (bw <= 40) {
		r &= (bwr << 32) + 0xFFFFFFFFUL;
	} else if (bw <= 48) {
		r &= (bwr << 40) + 0xFFFFFFFFFFUL;
	} else if (bw <= 56) {
		r &= (bwr << 48) + 0xFFFFFFFFFFFFUL;
	} else if (bw <= 64) {
		r &= (bwr << 56) + 0xFFFFFFFFFFFFUL;
	}
	return r;
}

template<int idx, typename L, typename T> void decode(T& t, std::uint64_t bits) {
	t = (T)bits;
}

template <int c, typename... Ts, typename U, typename... Us> void extract_helper(char* buffer, frame_layout<Ts ...> const & l,U& u, Us & ... us) {
	auto t = get_value<frame_layout<Ts ...> ,c>(buffer);
	decode<c, frame_layout<Ts ...> >(u, t);
	extract_helper<c + 1>(buffer,l,us...);
}

template <int c, typename... Ts, typename U> void extract_helper(char* buffer, frame_layout<Ts ...> const & l, U& u) {
	auto t = get_value<frame_layout<Ts ...>, c>(buffer);
	decode< c, frame_layout<Ts ...> >(u, t);
}

template <typename... Ts, typename... Us> void extract(char* buffer,frame_layout<Ts ...> const & l, Us & ... us) {
	extract_helper<0>(buffer,l, us...);
}

