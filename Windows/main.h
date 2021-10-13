#pragma once

#include "ppsspp_config.h"
#include "Debugger/Debugger_Disasm.h"
#include "Debugger/Debugger_MemoryDlg.h"
#include "Common/CommonWindow.h"

extern CDisasm *disasmWindow;
extern CMemoryDlg* memoryWindow;

#if PPSSPP_API(ANY_GL)
#include "Windows/GEDebugger/GEDebugger.h"
extern CGEDebugger* geDebuggerWindow;
#endif

extern HMENU g_hPopupMenus;
extern int g_activeWindow;

enum { WINDOW_MAINWINDOW, WINDOW_CPUDEBUGGER, WINDOW_GEDEBUGGER };
