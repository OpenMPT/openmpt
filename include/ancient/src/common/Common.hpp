/* Copyright (C) Teemu Suutari */

#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

#include <string>
#include <vector>

namespace ancient::internal
{

constexpr uint16_t MultiChar2(const char (&cc)[3]) noexcept
{
	return static_cast<uint16_t>((static_cast<uint8_t>(cc[0]) << 8) | static_cast<uint8_t>(cc[1]));
}

constexpr uint32_t FourCC(const char (&cc)[5]) noexcept
{
	return static_cast<uint32_t>((static_cast<uint8_t>(cc[0]) << 24) | (static_cast<uint8_t>(cc[1]) << 16) | (static_cast<uint8_t>(cc[2]) << 8) | static_cast<uint8_t>(cc[3]));
}

constexpr bool isValidSize(uint64_t &value) noexcept
{
#if INTPTR_MAX == INT32_MAX
	return value<0x1'0000'0000ULL;
#else
	return true;
#endif
}

constexpr bool isValidSize(off_t &value) noexcept
{
#if INTPTR_MAX == INT32_MAX
	return value<0x1'0000'0000ULL;
#else
	return true;
#endif
}

uint32_t rotateBits(uint32_t value,uint32_t count);

}

#endif
