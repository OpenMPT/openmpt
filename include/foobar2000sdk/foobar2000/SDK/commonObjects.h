#pragma once

#include <functional>

namespace fb2k {
	typedef service_ptr objRef;

	objRef callOnRelease( std::function< void () > );
    objRef callOnReleaseInMainThread( std::function< void () > );
}
