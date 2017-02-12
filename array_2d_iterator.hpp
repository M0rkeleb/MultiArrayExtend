#pragma once

#include <boost/multi_array.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <type_traits>

template <class T>
class array_2d_iterator : public boost::iterator_facade<array_2d_iterator<T>,T,boost::random_access_traversal_tag>
{
public:
	array_2d_iterator() : m_ptr(nullptr), m_dir('h'), a_width(0) { a_loc.first = 0; a_loc.second = 0; }
	array_2d_iterator(T *p) : m_ptr(p), m_dir('h'), a_width(0) { a_loc.first = 0; a_loc.second = 0; }
	array_2d_iterator(T *p, char d, std::size_t w, std::size_t i = 0, std::size_t j = 0) : m_ptr(p), m_dir(d), a_width(w) { a_loc.first = i; a_loc.second = j; }
	template <class U, std::enable_if_t<std::is_convertible<U*, T*>::value,int> = 0>
	array_2d_iterator(array_2d_iterator<U> const& other) : m_ptr(other.m_ptr), m_dir(other.m_dir), a_width(other.a_width), a_loc(other.a_loc) {}
private:
	T *m_ptr;
	char m_dir;
	// Direction of this iterator - four possibilities.
	// h = horizontal
	// v = vertical
	// d = descending (diagonally)
	// a = ascending (diagonally)
	std::pair<std::size_t, std::size_t> a_loc; //location in the array, needed to be able to compare iterators on different slices and for difference
	std::size_t a_width; //width of the 2d array, needed to traverse with non-horizontal iterators
	friend class boost::iterator_core_access;
	template <class> friend class array_2d_iterator;
	void increment() {
		switch (m_dir)
		{
		case 'h':
			m_ptr++; a_loc.second++; break;
		case 'v':
			m_ptr += a_width; a_loc.first++; break;
		case 'd':
			m_ptr += a_width; m_ptr++; 
			a_loc.first++; a_loc.second++; break;
		case 'a':
			m_ptr += a_width; m_ptr--;
			a_loc.second--; a_loc.first++; break;
		default:
			break;
		}
	}
	void decrement() {
		switch (m_dir)
		{
		case 'h':
			m_ptr--; a_loc.second--; break;
		case 'v':
			m_ptr -= a_width; a_loc.first--; break;
		case 'd':
			m_ptr -= a_width; m_ptr--;
			a_loc.first--; a_loc.second--; break;
		case 'a':
			m_ptr -= a_width; m_ptr++;
			a_loc.second++; a_loc.first--; break;
		default:
			break;
		}
	}
	template <class U>
	bool equal(array_2d_iterator<U> const& other) const {
		//Trying to compare iterators like these gets very fuzzy
		//Main intention will be to have a 2dbegin and 2dend at opposite corners and 
		//let x == 2dbegin/2dend mean x cannot be decremented/incremented anymore
		//Aside from that this is a pretty useless function
		//The resulting operation is not commutative or associative 
		switch (m_dir)
		{
		case 'h':
			return (a_loc.second == other.a_loc.second);
		case 'v':
			return (a_loc.first == other.a_loc.first);
		case 'd':
			return (a_loc.second == other.a_loc.second || a_loc.first == other.a_loc.first);
		case 'a':
			return (a_loc.second + other.a_loc.second + 1 == a_width || a_loc.first == other.a_loc.first);
		default:
			return true;
		}
	}
	void advance(ptrdiff_t n) {
		for (ptrdiff_t i = 0; i < n; i++) { increment(); }
	}
	template <class U>
	ptrdiff_t distance_to(array_2d_iterator<U> const& other) const {
		//Another fuzzy comparison!  This time x.distance_to(z) == n means x.advance(n).equal(z) == true
		//If x and z have the same direction this is straightforward
		//But in general also does not behave as expected with math (e.g. x.distance_to(z) might not be negative of z.distance_to(x) if they have different directions
		if (m_dir == 'h') { return static_cast<ptrdiff_t>(other.a_loc.second - a_loc.second); }
		return static_cast<ptrdiff_t>(other.a_loc.first - a_loc.first);
	}
	T& dereference() const { return *(this->m_ptr); }
};

template<typename T>
using array_2d = boost::multi_array<T, 2>;

template<typename T>
array_2d_iterator<T> two_d_begin(array_2d<T> &source, char dir = 'h') {
	return array_2d_iterator<T>(source.origin(), dir, source.shape()[1], 0, 0);
}

template<typename T>
array_2d_iterator<T> two_d_end(array_2d<T> &source, char dir = 'h') {
	return array_2d_iterator<T>(source.origin()+source.num_elements()-1, dir, source.shape()[1], source.shape()[0]-1, source.shape()[1]-1);
}

template<typename T>
array_2d_iterator<T> iter_from_coord(array_2d<T> &source, int i, int j, char dir = 'h') {
	auto width = source.shape()[1];
	return array_2d_iterator<T>(source.origin() + i*width + j, dir, width, i, j);
}

template<typename T>
array_2d_iterator<T const> ctwo_d_begin(array_2d<T> const &source, char dir = 'h') {
	return array_2d_iterator<T const>(source.origin(), dir, source.shape()[1], 0, 0);
}

template<typename T>
array_2d_iterator<T const> ctwo_d_end(array_2d<T> const &source, char dir = 'h') {
	return array_2d_iterator<T const>(source.origin() + source.num_elements() - 1, dir, source.shape()[1], source.shape()[0] - 1, source.shape()[1] - 1);
}

template<typename T>
array_2d_iterator<T const> citer_from_coord(array_2d<T> const &source, int i, int j, char dir = 'h') {
	auto width = source.shape()[1];
	return array_2d_iterator<T const>(source.origin() + i*width + j, dir, width, i, j);
}