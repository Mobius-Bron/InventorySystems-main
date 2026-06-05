#pragma once

namespace DH
{
	DEBUGHELPER_API bool IsDebugEnabled();
}

#if UE_BUILD_SHIPPING
	#define DH_ENABLED (false)
#else
	#define DH_ENABLED (DH::IsDebugEnabled())
#endif
