#pragma once

#include "Engine/pch.h"

#include <limits>

#pragma warning(push)
#pragma warning(disable: 4819 4244)
#if defined(CSG_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wmultichar"
#endif  // defined(CSG_COMPILER_CLANG)

#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <FL/fl_string_functions.h>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/platform.H>
#include <errno.h>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Input.H>
#include <FL/x.H>
#include <FL/Fl_Scroll.H>

#if defined(CSG_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif  // defined(CSG_COMPILER_CLANG)
#pragma warning(pop)