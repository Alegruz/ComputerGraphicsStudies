#pragma once

#include <concepts>

template<typename T>
concept CFloatType = std::is_floating_point<T>::value;

template<typename T>
concept CArithmeticType = std::is_arithmetic<T>::value;
