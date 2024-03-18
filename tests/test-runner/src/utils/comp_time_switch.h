/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef UTILS_COMP_TIME_SWITCH_H
#define UTILS_COMP_TIME_SWITCH_H 1

#include <cstdlib>
#include <utility>

template<typename T, T value>
struct TypeContainer {};

template<size_t I>
using size_t_ = TypeContainer<size_t, I>;

template<size_t N = 0, typename T, T I>
constexpr size_t get_sequence_index(const T &value, size_t fallback, std::integer_sequence<T, I>) {
	return (value == I) ? N : fallback;
}

template<size_t N = 0, typename T, T I, T... Is>
constexpr size_t get_sequence_index(const T &value, size_t fallback, std::integer_sequence<T, I, Is...>) {
	return (value == I) ? N : get_sequence_index<N + 1>(value, fallback, std::integer_sequence<T, Is...>{});
}

template <size_t N = 0, class T, T I, class F, class F2>
constexpr std::result_of_t<F(T)> constexpr_switch_impl(const T &value, std::integer_sequence<T, I>, F f, F2 fallback) {
	return (value == I) ? f(size_t_<I>(), size_t_<N>()) : fallback();
}

template <size_t N = 0, class T, T I, T ... Is, class F, class F2>
constexpr std::result_of_t<F(T)> constexpr_switch_impl(const T &value, std::integer_sequence<T, I, Is...>, F f, F2 fallback) {
	return (value == I) ? f(size_t_<I>(), size_t_<N>()) : constexpr_switch_impl<N + 1>(value, std::integer_sequence<T, Is...>{}, f, fallback);
}

template <class T, T I, class F, class F2>
constexpr std::result_of_t<F(T)> compile_switch_impl(const T &value, std::integer_sequence<T, I>, F f, F2 fallback) {
	return (value == I) ? f(I) : fallback();
}

template <class T, T I, T ... Is, class F, class F2>
constexpr std::result_of_t<F(T)> compile_switch_impl(const T &value, std::integer_sequence<T, I, Is...>, F f, F2 fallback) {
	return (value == I) ? f(I) : compile_switch_impl(value, std::integer_sequence<T, Is...>{}, f, fallback);
}

template <class T, T ... Is>
constexpr size_t compile_switch_idx(const T &i, size_t fallback, std::integer_sequence<T, Is...> seq) {
	return get_sequence_index(i, fallback, seq);
}

template <class T, T ... Is, class F, class F2>
constexpr std::result_of_t<F(const T &)> constexpr_switch(const T &value, std::integer_sequence<T, Is...> seq, F f, F2 fallback) {
	return constexpr_switch_impl(value, seq, f, fallback);
}

template <class T, T ... Is, class F, class F2>
constexpr std::result_of_t<F(const T &)> compile_switch(const T &value, std::integer_sequence<T, Is...> seq, F f, F2 fallback) {
	return compile_switch_impl(value, seq, f, fallback);
}

#endif /* UTILS_COMP_TIME_SWITCH_H */
