#include "foobar2000.h"

GUID hasher_md5::guid_from_result(const hasher_md5_result & param)
{
	assert(sizeof(GUID) == sizeof(hasher_md5_result));
	GUID temp = * reinterpret_cast<const GUID*>(&param);
	byte_order::order_le_to_native_t(temp);
	return temp;
}

hasher_md5_result hasher_md5::process_single(const void * p_buffer,t_size p_bytes)
{
	hasher_md5_state state;
	initialize(state);
	process(state,p_buffer,p_bytes);
	return get_result(state);
}

GUID hasher_md5::process_single_guid(const void * p_buffer,t_size p_bytes)
{
	return guid_from_result(process_single(p_buffer,p_bytes));
}

t_uint64 hasher_md5_result::xorHalve() const {
#if PFC_BYTE_ORDER_IS_BIG_ENDIAN
	t_uint64 ret = 0;
	for(int walk = 0; walk < 8; ++walk) {
		ret |= (t_uint64)((t_uint8)m_data[walk] ^ (t_uint8)m_data[walk+8]) << (walk * 8);
	}
	return ret;
#else
	const t_uint64 * v = reinterpret_cast<const t_uint64*>(&m_data);
	return v[0] ^ v[1];
#endif
}
