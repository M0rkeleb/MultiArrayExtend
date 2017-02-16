#pragma once

#include <boost/multi_array.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <type_traits>

template<typename T>
using array_2d = boost::multi_array<T, 2>;

using size_t_pair = std::pair<std::size_t, std::size_t>;

inline void move_fwd(char axis, size_t_pair& locat) {
	switch (axis)
	{
	case 'h':
		locat.second++; break;
	case 'v':
		locat.first++; break;
	case 'd':
		locat.first++; locat.second++; break;
	case 'a':
		locat.second--; locat.first++; break;
	default:
		break;
	}
}

inline void move_back(char axis, size_t_pair& locat) {
	switch (axis)
	{
	case 'h':
		locat.second--; break;
	case 'v':
		locat.first--; break;
	case 'd':
		locat.first--; locat.second--; break;
	case 'a':
		locat.second++; locat.first--; break;
	default:
		break;
	}
}

inline bool equal_loc(char axis, size_t_pair loc_1, size_t_pair loc_2, std::size_t width) {
	//used to compare iterators
	//Trying to compare iterators like these gets very fuzzy
	//Main intention will be to have a 2dbegin and 2dend at opposite corners and 
	//to provide the same kind of boundaries begin and end do for regular containers
	//Aside from that this is a pretty useless function
	//The resulting operation is not commutative or associative 
	switch (axis)
	{
	case 'h':
		return (loc_1.second == loc_2.second);
	case 'v':
		return (loc_1.first == loc_2.first);
	case 'd':
		return (loc_1.second == loc_2.second || loc_1.first == loc_2.first);
	case 'a':
		return (loc_1.second + loc_2.second == width + 1 || loc_1.first == loc_2.first);
	default:
		return true;
	}
}

template <class T>
class array_2d_iterator : public boost::iterator_facade<array_2d_iterator<T>,T,boost::random_access_traversal_tag>
{
public:
	array_2d_iterator(array_2d<T>& source, std::size_t i, std::size_t j) : m_source(source), m_axis('h') { a_loc.first = i; a_loc.second = j; }
	array_2d_iterator(array_2d<T>& source, std::size_t i, std::size_t j, char d) : m_source(source), m_axis(d) { a_loc.first = i; a_loc.second = j; }
	template <class U, std::enable_if_t<std::is_convertible<U*, T*>::value,int> = 0>
	array_2d_iterator(array_2d_iterator<U> const& other) : m_source(other.m_source), m_axis(other.m_axis), a_loc(other.a_loc) {}
private:
	array_2d<T>& m_source;
	char m_axis;
	// Direction of this iterator - four possibilities.
	// h = horizontal
	// v = vertical
	// d = descending (diagonally)
	// a = ascending (diagonally)
	std::pair<std::size_t, std::size_t> a_loc; 
	//location in the array - but starting at 1,1 to be able to be consistent with reverse iterator so rend can have index 0,0
	//needed to be able to compare iterators on different slices and for difference
	std::size_t a_width() const { return m_source.shape()[1]; } //width of the 2d array, helpful to traverse with non-horizontal iterators
	friend class boost::iterator_core_access;
	template <class> friend class array_2d_iterator;
	void increment() { move_fwd(m_axis, a_loc); }
	void decrement() { move_back(m_axis, a_loc); }
	bool equal(array_2d_iterator<T> const& other) const {
		if (m_source != other.m_source) { return false; }
		return equal_loc(m_axis, a_loc, other.a_loc, this->a_width());
	}
	void advance(ptrdiff_t n) {
		for (ptrdiff_t i = 0; i < n; i++) { increment(); }
	}
	ptrdiff_t distance_to(array_2d_iterator<T> const& other) const {
		//Another fuzzy comparison!  This time x.distance_to(z) == n means x.advance(n).equal(z) == true
		//If x and z have the same direction this is straightforward
		//But in general also does not behave as expected with math (e.g. x.distance_to(z) might not be negative of z.distance_to(x) if they have different directions
		if (m_axis == 'h') { return static_cast<ptrdiff_t>(other.a_loc.second) - static_cast<ptrdiff_t>(a_loc.second); }
		return static_cast<ptrdiff_t>(other.a_loc.first) - static_cast<ptrdiff_t>(a_loc.first);
	}
	T& dereference() const { return m_source[a_loc.first - 1][a_loc.second - 1]; }
};

//Convenience functions for begin, end, and arbitrary index iterator creators from a given 2d_array

template<typename T>
array_2d_iterator<T> two_d_begin(array_2d<T> &source, char dir = 'h') {
	return array_2d_iterator<T>(source, 1, 1, dir);
}

template<typename T>
array_2d_iterator<T> two_d_end(array_2d<T> &source, char dir = 'h') {
	return array_2d_iterator<T>(source, source.shape()[0] + 1, source.shape()[1] + 1, dir);
}

template<typename T>
array_2d_iterator<T> iter_from_coord(array_2d<T> &source, std::size_t i, std::size_t j, char dir = 'h') {
	//coord relative to usual array indexing (assuming index starts at 0,0 .. could generalize later
	return array_2d_iterator<T>(source, i + 1, j + 1, dir);
}

template <class T>
class const_array_2d_iterator : public boost::iterator_facade<const_array_2d_iterator<T>, T const, boost::random_access_traversal_tag>
{
public:
	const_array_2d_iterator(array_2d<T> const& source, std::size_t i, std::size_t j) : m_source(source), m_axis('h') { a_loc.first = i; a_loc.second = j; }
	const_array_2d_iterator(array_2d<T> const& source, std::size_t i, std::size_t j, char d) : m_source(source), m_axis(d) { a_loc.first = i; a_loc.second = j; }
	template <class U, std::enable_if_t<std::is_convertible<U*, T*>::value, int> = 0>
	const_array_2d_iterator(const_array_2d_iterator<U> const& other) : m_source(other.m_source), m_axis(other.m_axis), a_loc(other.a_loc) {}
private:
	array_2d<T> const& m_source;
	char m_axis;
	std::pair<std::size_t, std::size_t> a_loc;
	std::size_t a_width() const { return m_source.shape()[1]; } 
	friend class boost::iterator_core_access;
	template <class> friend class array_2d_iterator;
	void increment() { move_fwd(m_axis, a_loc); }
	void decrement() { move_back(m_axis, a_loc); }
	bool equal(const_array_2d_iterator<T> const& other) const {
		if (m_source != other.m_source) { return false; }
		return equal_loc(m_axis, a_loc, other.a_loc, this->a_width());
	}
	void advance(ptrdiff_t n) {
		for (ptrdiff_t i = 0; i < n; i++) { increment(); }
	}
	ptrdiff_t distance_to(const_array_2d_iterator<T> const& other) const {
		if (m_axis == 'h') { return static_cast<ptrdiff_t>(other.a_loc.second) - static_cast<ptrdiff_t>(a_loc.second); }
		return static_cast<ptrdiff_t>(other.a_loc.first) - static_cast<ptrdiff_t>(a_loc.first);
	}
	T const& dereference() const { return m_source[a_loc.first - 1][a_loc.second - 1]; }
};




//Create const interators .. I hope

template<typename T>
const_array_2d_iterator<T> two_d_begin(array_2d<T> const &source, char dir = 'h') {
	return const_array_2d_iterator<T>(source, 1, 1, dir);
}

template<typename T>
const_array_2d_iterator<T> two_d_end(array_2d<T> const &source, char dir = 'h') {
	return const_array_2d_iterator<T>(source, source.shape()[0] + 1, source.shape()[1] + 1, dir);
}

template<typename T>
const_array_2d_iterator<T> iter_from_coord(array_2d<T> const &source, std::size_t i, std::size_t j, char dir = 'h') {
	//coord relative to usual array indexing (assuming index starts at 0,0 .. could generalize later
	return const_array_2d_iterator<T>(source, i + 1, j + 1, dir);
}

template<typename T>
const_array_2d_iterator<T> ctwo_d_begin(array_2d<T> const &source, char dir = 'h') {
	return two_d_begin(source, dir);
}

template<typename T>
const_array_2d_iterator<T> ctwo_d_end(array_2d<T> const &source, char dir = 'h') {
	return two_d_end(source, dir);
}

template<typename T>
const_array_2d_iterator<T> citer_from_coord(array_2d<T> const &source, std::size_t i, std::size_t j, char dir = 'h') {
	//coord relative to usual array indexing (assuming index starts at 0,0 .. could generalize later
	return iter_from_coord(source, i, j, dir);
}

template <class T>
class rev_array_2d_iterator : public boost::iterator_facade<rev_array_2d_iterator<T>, T, boost::random_access_traversal_tag>
{
public:
	rev_array_2d_iterator(array_2d<T>& source, std::size_t i, std::size_t j) : m_source(source), m_axis('h') { a_loc.first = i; a_loc.second = j; }
	rev_array_2d_iterator(array_2d<T>& source, std::size_t i, std::size_t j, char d) : m_source(source), m_axis(d) { a_loc.first = i; a_loc.second = j; }
	template <class U, std::enable_if_t<std::is_convertible<U*, T*>::value, int> = 0>
	rev_array_2d_iterator(rev_array_2d_iterator<U> const& other) : m_source(other.m_source), m_axis(other.m_axis), a_loc(other.a_loc) {}
private:
	array_2d<T>& m_source;
	char m_axis;
	std::pair<std::size_t, std::size_t> a_loc;
	std::size_t a_width() const { return m_source.shape()[1]; } 
	friend class boost::iterator_core_access;
	template <class> friend class rev_array_2d_iterator;
	void increment() { move_back(m_axis, a_loc); }
	void decrement() { move_fwd(m_axis, a_loc); }
	bool equal(rev_array_2d_iterator<T> const& other) const {
		if (m_source != other.m_source) { return false; }
		return equal_loc(m_axis, a_loc, other.a_loc, this->a_width());
	}
	void advance(ptrdiff_t n) {
		for (ptrdiff_t i = 0; i < n; i++) { increment(); }
	}
	ptrdiff_t distance_to(rev_array_2d_iterator<T> const& other) const {
		if (m_axis == 'h') { return static_cast<ptrdiff_t>(a_loc.second) - static_cast<ptrdiff_t>(other.a_loc.second); }
		return static_cast<ptrdiff_t>(a_loc.first) - static_cast<ptrdiff_t>(other.a_loc.first);
	}
	T& dereference() const { return m_source[a_loc.first - 1][a_loc.second - 1]; }
};

//Convenience functions for begin, end, and arbitrary index iterator creators from a given 2d_array

template<typename T>
rev_array_2d_iterator<T> rtwo_d_begin(array_2d<T> &source, char dir = 'h') {
	return rev_array_2d_iterator<T>(source, source.shape()[0], source.shape()[1], dir);
}

template<typename T>
rev_array_2d_iterator<T> rtwo_d_end(array_2d<T> &source, char dir = 'h') {
	return rev_array_2d_iterator<T>(source, 0, 0, dir);
}

template<typename T>
rev_array_2d_iterator<T> riter_from_coord(array_2d<T> &source, std::size_t i, std::size_t j, char dir = 'h') {
	//coord relative to usual array indexing (assuming index starts at 0,0 .. could generalize later
	return rev_array_2d_iterator<T>(source, i + 1, j + 1, dir);
}

template <class T>
class const_rev_array_2d_iterator : public boost::iterator_facade<const_rev_array_2d_iterator<T>, T const, boost::random_access_traversal_tag>
{
public:
	const_rev_array_2d_iterator(array_2d<T> const& source, std::size_t i, std::size_t j) : m_source(source), m_axis('h') { a_loc.first = i; a_loc.second = j; }
	const_rev_array_2d_iterator(array_2d<T> const& source, std::size_t i, std::size_t j, char d) : m_source(source), m_axis(d) { a_loc.first = i; a_loc.second = j; }
	template <class U, std::enable_if_t<std::is_convertible<U*, T*>::value, int> = 0>
	const_rev_array_2d_iterator(const_array_2d_iterator<U> const& other) : m_source(other.m_source), m_axis(other.m_axis), a_loc(other.a_loc) {}
private:
	array_2d<T> const& m_source;
	char m_axis;
	std::pair<std::size_t, std::size_t> a_loc;
	std::size_t a_width() const { return m_source.shape()[1]; }
	friend class boost::iterator_core_access;
	template <class> friend class array_2d_iterator;
	void increment() { move_back(m_axis, a_loc); }
	void decrement() { move_fwd(m_axis, a_loc); }
	bool equal(const_rev_array_2d_iterator<T> const& other) const {
		if (m_source != other.m_source) { return false; }
		return equal_loc(m_axis, a_loc, other.a_loc, this->a_width());
	}
	void advance(ptrdiff_t n) {
		for (ptrdiff_t i = 0; i < n; i++) { increment(); }
	}
	ptrdiff_t distance_to(const_rev_array_2d_iterator<T> const& other) const {
		if (m_axis == 'h') { return static_cast<ptrdiff_t>(a_loc.second) - static_cast<ptrdiff_t>(other.a_loc.second); }
		return static_cast<ptrdiff_t>(a_loc.first) - static_cast<ptrdiff_t>(other.a_loc.first);
	}
	T const& dereference() const { return m_source[a_loc.first - 1][a_loc.second - 1]; }
};




//Create const interators .. I hope

template<typename T>
const_rev_array_2d_iterator<T> two_d_begin(array_2d<T> const &source, char dir = 'h') {
	return const_rev_array_2d_iterator<T>(source, source.shape()[0], source.shape()[1], dir);
}

template<typename T>
const_rev_array_2d_iterator<T> two_d_end(array_2d<T> const &source, char dir = 'h') {
	return const_rev_array_2d_iterator<T>(source, 0, 0, dir);
}

template<typename T>
const_rev_array_2d_iterator<T> iter_from_coord(array_2d<T> const &source, std::size_t i, std::size_t j, char dir = 'h') {
	//coord relative to usual array indexing (assuming index starts at 0,0 .. could generalize later
	return const_rev_array_2d_iterator<T>(source, i + 1, j + 1, dir);
}

template<typename T>
const_rev_array_2d_iterator<T> crtwo_d_begin(array_2d<T> const &source, char dir = 'h') {
	return two_d_begin(source, dir);
}

template<typename T>
const_rev_array_2d_iterator<T> crtwo_d_end(array_2d<T> const &source, char dir = 'h') {
	return two_d_end(source, dir);
}

template<typename T>
const_rev_array_2d_iterator<T> criter_from_coord(array_2d<T> const &source, std::size_t i, std::size_t j, char dir = 'h') {
	//coord relative to usual array indexing (assuming index starts at 0,0 .. could generalize later
	return iter_from_coord(source, i, j, dir);
}