#include "Launcher/pch.h"

#include "Launcher/Launcher.h"

int main(int argc, char **argv)
{
	cgs::Launcher launcher;

	return launcher.Run(argc, argv);
}
