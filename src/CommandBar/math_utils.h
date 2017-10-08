#pragma once

namespace math
{
	template<typename T>
	T max(const T& a, const T& b)
	{
		if (a >= b) return a;
		return b;
	}

	template<typename T>
	T min(const T& a, const T& b)
	{
		if (a <= b) return a;
		return b;
	}
}