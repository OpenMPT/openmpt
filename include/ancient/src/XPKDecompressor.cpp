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

void XPKDecompressor::registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<XPKDecompressor>(*create)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool))
{
	XPKMain::registerDecompressor(detect,create);
}

}
