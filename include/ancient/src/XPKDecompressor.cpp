/* Copyright (C) Teemu Suutari */

#include "XPKDecompressor.hpp"
#include "XPKMain.hpp"

namespace ancient::internal
{

XPKDecompressor::State::~State()
{
	// nothing needed
}

XPKDecompressor::XPKDecompressor(uint32_t recursionLevel) :
	_recursionLevel(recursionLevel)
{
	// nothing needed
}

XPKDecompressor::~XPKDecompressor()
{
	// nothing needed
}

}
