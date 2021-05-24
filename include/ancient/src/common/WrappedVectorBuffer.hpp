/* Copyright (C) Teemu Suutari */

#ifndef WRAPPEDVECTORBUFFER_HPP
#define WRAPPEDVECTORBUFFER_HPP

#include <vector>

#include <cstddef>
#include <cstdint>

#include "Buffer.hpp"

namespace ancient::internal
{

class WrappedVectorBuffer : public Buffer
{
public:
	WrappedVectorBuffer(std::vector<uint8_t> &refdata);
	virtual ~WrappedVectorBuffer() override final;

	virtual const uint8_t *data() const noexcept override final;
	virtual uint8_t *data() override final;
	virtual size_t size() const noexcept override final;

	virtual bool isResizable() const noexcept override final;
	virtual void resize(size_t newSize) override final;

private:
	std::vector<uint8_t> & _refdata;
};

}

#endif
