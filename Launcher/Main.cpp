#include "Launcher/pch.h"

#include "Launcher/Launcher.h"

int main(int argc, char **argv)
{
	int result = 0;
	{
		cgs::Launcher launcher;

		result = launcher.Run(argc, argv);
	}
#if defined(_CRTDBG_MAP_ALLOC)
	_CrtDumpMemoryLeaks(); // Check for memory leaks if using MSVC
#endif	// defined(_CRTDBG_MAP_ALLOC)
	return result;
}
