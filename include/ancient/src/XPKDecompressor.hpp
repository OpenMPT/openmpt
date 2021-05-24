/* Copyright (C) Teemu Suutari */

#ifndef XPKDECOMPRESSOR_HPP
#define XPKDECOMPRESSOR_HPP

#include <cstddef>
#include <cstdint>

#include <string>

#include "Decompressor.hpp"

namespace ancient::internal
{

class XPKDecompressor
{
public:
	class State
	{
	public:
		State(const State&)=delete;
		State& operator=(const State&)=delete;

		State()=default;
		virtual ~State();

		uint32_t getRecursionLevel() const;
	};

	XPKDecompressor(const XPKDecompressor&)=delete;
	XPKDecompressor& operator=(const XPKDecompressor&)=delete;

	XPKDecompressor(uint32_t recursionLevel=0);
	virtual ~XPKDecompressor();

	virtual const std::string &getSubName() const noexcept=0;

	// Actual decompression
	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)=0;

	template<class T>
	class Registry
	{
	public:
		Registry()
		{
			XPKDecompressor::registerDecompressor(T::detectHeaderXPK,T::create);
		}

		~Registry()
		{
			// TODO: no cleanup yet
		}
	};

private:
	static void registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<XPKDecompressor>(*create)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool));

protected:
	uint32_t	_recursionLevel;
};

}

#endif
