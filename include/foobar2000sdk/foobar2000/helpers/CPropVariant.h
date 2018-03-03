#pragma once

class CPropVariant : public PROPVARIANT {
public:
	CPropVariant() {init();}
	~CPropVariant() {clear();}
	CPropVariant( const CPropVariant & other ) {
		init();
		PropVariantCopy( this, &other );
	}
	const CPropVariant& operator=( const CPropVariant & other ) {
		clear();
		PropVariantCopy(this, &other);
		return *this;
	}

	bool toInt64(int64_t & out) const {
		switch( vt ) {
		case VT_I1: out = (int64_t) cVal; return true;
		case VT_I2: out = (int64_t) iVal; return true;
		case VT_I4: out = (int64_t) lVal; return true;
		case VT_I8: out = (int64_t) hVal.QuadPart; return true;
		case VT_INT: out = (int64_t) intVal; return true;
		default: return false;
		}
	}
	bool toUint64(uint64_t & out) const {
		switch( vt ) {
		case VT_UI1: out = (uint64_t) bVal; return true;
		case VT_UI2: out = (uint64_t) uiVal; return true;
		case VT_UI4: out = (uint64_t) ulVal; return true;
		case VT_UI8: out = (uint64_t) uhVal.QuadPart; return true;
		case VT_UINT: out = (uint64_t) uintVal; return true;
		default: return false;
		}
	}
	bool toString( pfc::string_base & out ) const {
		switch( vt ) {
		case VT_LPSTR:
			out = pfc::stringcvt::string_utf8_from_ansi( pszVal ); return true;
		case VT_LPWSTR:
			out = pfc::stringcvt::string_utf8_from_wide( pwszVal ); return true;
		default: return false;
		}
	}
private:
	void clear() {
		PropVariantClear( this );
	}
	void init() {
		PROPVARIANT * pv = this;
		PropVariantInit( pv );
	}
};
