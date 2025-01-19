#include <iostream>

#include "Core/PchCommon.h"

#include "Renderer/RendererMain.h"

int main()
{
	cgs::core::ProjectInfo applicationInfo =
	{
		.Name = "RendererMain",
		.Version = MAKE_API_VERSION(0, 0, 0, 1),
	};

	const int result = RendererMain(applicationInfo);
	std::cout << "result: " << result << std::endl;

	return 0;
}