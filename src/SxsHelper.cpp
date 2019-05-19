#include "SxsHelper.h"

CAppModule _Module;

int Run(int nCmdShow)
{
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    CMainFrm wndMain;
    HWND hWnd = wndMain.CreateEx(HWND_DESKTOP);
    if (NULL == hWnd) return ::GetLastError();
    wndMain.ShowWindow(nCmdShow);

    wndMain.SendMessage(WM_COMMAND, ID_FILE_MRU_FIRST);

    int nRet = theLoop.Run();
    _Module.RemoveMessageLoop();
    return nRet;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ATLASSERT(SUCCEEDED(hr));

    hr = AtlInitCommonControls(ICC_TREEVIEW_CLASSES);
    ATLASSERT(SUCCEEDED(hr));

    hr = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hr));

    int nRet = Run(nCmdShow);

    _Module.Term();

    ::CoUninitialize();
    return nRet;
}