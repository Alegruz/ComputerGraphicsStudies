#include "Launcher/pch.h"

#include "Launcher/Launcher.h"

int main(int argc, char **argv)
{
	// cgs::Engine::CreateInfo engineCreateInfo =
	// {
	// 	.ConfigCreateInfo =
	// 	{
	// 		.ConfigFilePath = "config.ini" // Path to the configuration file
	// 	}
	// };
	// cgs::Engine engine(engineCreateInfo);

	cgs::Launcher launcher;

	return launcher.Run(argc, argv);
}
