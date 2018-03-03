#pragma once

namespace pfc {
	void crashImpl();
	void crashHook() {crashImpl();}

	BOOL winFormatSystemErrorMessageImpl(pfc::string_base & p_out, DWORD p_code);

	BOOL winFormatSystemErrorMessageHook(pfc::string_base & p_out, DWORD p_code) {
		return winFormatSystemErrorMessageImpl(p_out, p_code);
	}
}