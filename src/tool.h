#include <cstddef>   // 用于 size_t
#include <algorithm> // 用于 std::equal

template <typename T, std::size_t N>
bool areArraysEqual(const T (&a)[N], const T (&b)[N])
{
  return std::equal(a, a + N, b);
};
