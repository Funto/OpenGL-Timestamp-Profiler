// hole_array.h

#ifndef HOLE_ARRAY_H
#define HOLE_ARRAY_H

#include <stddef.h>
#include <assert.h>
#include <vector>

template<class T, const size_t max_size>
class HoleArray
{
public:
	static const size_t	MAX_SIZE = max_size;
private:
	T					m_elements[MAX_SIZE];
	std::vector<bool>	m_used;	// vector<bool> is optimized for space
	volatile size_t		m_size;
public:

	HoleArray() : m_used(MAX_SIZE, false), m_size(0)
	{
	}

	size_t		getSize()const		{return m_size;}
	size_t		getMaxSize() const	{return MAX_SIZE;}

	T*			getPtr()		{return m_elements;}
	const T*	getPtr() const	{return m_elements;}

	T&			operator[](size_t i)		{ assert(m_used[i]);	return m_elements[i];}
	const T&	operator[](size_t i) const	{ assert(m_used[i]);	return m_elements[i];}

	T&			get(size_t i)		{ assert(m_used[i]);	return m_elements[i];}
	const T&	get(size_t i) const	{ assert(m_used[i]);	return m_elements[i];}

	bool		isUsed(size_t i) const	{return m_used[i];}

	size_t	add()
	{
		assert(m_size+1 < MAX_SIZE);

		size_t i = 0;
		while(m_used[i])
			i++;
		m_used[i] = true;
		m_size++;
		return i;
	}

	void	remove(size_t i)
	{
		assert(i < MAX_SIZE);
		m_used[i] = false;
		m_size--;
	}

	// Usage: for(size_t i = array.begin(); i != array.getMaxSize() ; i = array.next(i))
	size_t	begin() const
	{
		if(m_size == 0)
			return MAX_SIZE;

		size_t i = 0;
		while(i < MAX_SIZE && !m_used[i])
			i++;
		return i;
	}

	size_t	next(size_t i) const
	{
		i++;
		while(i < MAX_SIZE && !m_used[i])
			i++;
		return i;
	}
};

#endif // HOLE_ARRAY_H
