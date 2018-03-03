//! PRNG service. Implemented by the core, do not reimplement.  Use g_create() helper function to instantiate.
class NOVTABLE genrand_service : public service_base
{
public:
	//! Seeds the PRNG with specified value.
	virtual void seed(unsigned val) = 0;
	//! Returns random value N, where 0 <= N < range.
	virtual unsigned genrand(unsigned range)=0;

	double genrand_f() { return (double)genrand(0xFFFFFFFF) / (double)0xFFFFFFFF; }

	void genrand_blob( void * out, size_t bytes ) {
		size_t dwords = bytes/4;
		uint32_t * out32 = (uint32_t*) out;
		for(size_t w = 0; w < dwords; ++w ) {
			out32[w] = genrand32();
		}
		size_t left = bytes % 4;
		if (left > 0) {
			auto leftptr = (uint8_t*) out + (bytes-left);
			for( size_t w = 0; w < left; ++w) leftptr[w] = genrand8();
		}
	}

	uint32_t genrand32() {
		return (uint32_t) genrand(0xFFFFFFFF);
	}
	uint8_t genrand8() {
		return (uint8_t) genrand(0x100);
	}

	static service_ptr_t<genrand_service> g_create() {return standard_api_create_t<genrand_service>();}

	void generate_random_order(t_size * out, t_size count) {
        unsigned genrandMax = (unsigned) pfc::min_t<size_t>(count, 0xFFFFFFFF);
		t_size n;
		for(n=0;n<count;n++) out[n]=n;
		for(n=0;n<count;n++) pfc::swap_t(out[n],out[genrand(genrandMax)]);
	}

	FB2K_MAKE_SERVICE_COREAPI(genrand_service);
};
