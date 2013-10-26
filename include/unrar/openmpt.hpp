// OpenMPT definitions (so that unrar project settings are identical to settings when including rar.hpp)
#undef Log
#undef STRICT
#define UNRAR
#define SILENT
#define RARDLL
#define RAR_NOCRYPT
#define NOVOLUME
#define NOMINMAX	// For windows.h
