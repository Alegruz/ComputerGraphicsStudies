#include <iostream>

#include "Core/pch.h"

#include "Engine/Engine.h"

int main()
{
	cgs::Engine::CreateInfo engineCreateInfo =
	{
		.ConfigCreateInfo =
		{
			.ConfigFilePath = "config.ini" // Path to the configuration file
		}
	};
	cgs::Engine engine(engineCreateInfo);

	return 0;
}