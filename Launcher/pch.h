#pragma once

#include "Engine/pch.h"


#pragma warning(push)
#pragma warning(disable: 4819 4244)
#if defined(CSG_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wmultichar"
#endif  // defined(CSG_COMPILER_CLANG)

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>

#if defined(CSG_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif  // defined(CSG_COMPILER_CLANG)
#pragma warning(pop)