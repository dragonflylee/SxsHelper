#ifndef _STDAFX_H_
#define _STDAFX_H_

// ATL 头文件:
#include <atlimage.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlcoll.h>
#include <atlwin.h>

// Windows 头文件:
#include <shlobj.h>
#include <shlwapi.h>
#include <MsXml2.h>
#include <Commdlg.h>

#define HR_CHECK(_hr_) hr = _hr_; if (FAILED(hr)) { ATLTRACE(TEXT("0x%.8x\n"), hr); goto exit; }
#define BOOL_CHECK(_hr_) if (!(_hr_)) { hr = HRESULT_FROM_WIN32(::GetLastError()); ATLTRACE(TEXT("0x%.8x\n"), hr); goto exit; }

#include "Resource.h"
#include "FindDlg.h"
#include "MainFrm.h"

#endif // _STDAFX_H_