#ifndef _STDAFX_H_
#define _STDAFX_H_

#define _CRT_SECURE_NO_WARNINGS

// 改变这些值以适应不同的版本
#define WINVER		0x0601
#define _WIN32_WINNT	0x0601

// Windows 头文件:
#include <atlbase.h>
#include <atlstr.h>

extern CComModule _Module;

#include <atlcom.h> 
#include <atlhost.h> 

#include <atlwin.h>

#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") 
#pragma comment(lib, "Comctl32.lib") 

#include "StringHelp.h"
#include "Resource.h"
#include "AboutDlg.h"
#include "PkgTree.h"
#include "MainDlg.h"

#endif // _STDAFX_H_