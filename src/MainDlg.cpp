#include "StdAfx.h"
#include <MsXml2.h>

#define ID_APP_ABOUT                    0xE140

void LoadMainIcon(HWND hWnd)
{
    HMODULE hModule = LoadLibrary(TEXT("Shell32.dll"));

    if (NULL != hModule)
    {
        HICON hIcon = LoadIcon(hModule, MAKEINTRESOURCE(271));
        if (NULL != hIcon)
        {
            PostMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            PostMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        FreeLibrary(hModule);
    }
}

CMainDlg::CMainDlg() :
m_hThread(NULL),
m_szPath(NULL),
m_hRoot(NULL)
{

}

CMainDlg::~CMainDlg()
{

}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    m_szPath = (LPTSTR)lParam;
    // 设置对话框的图标
    LoadMainIcon(m_hWnd);

    // 设置系统菜单
    HMENU hSysMenu = GetSystemMenu(FALSE);
    InsertMenu(hSysMenu, 0, MF_SEPARATOR, ID_APP_ABOUT, NULL);
    InsertMenu(hSysMenu, 0, MF_STRING, ID_APP_ABOUT, CStr::Load(IDS_ABOUT));

    // 插入根节点
    TV_INSERTSTRUCT tvi;
    tvi.hParent = NULL;
    tvi.hInsertAfter = TVI_ROOT; // work as root level
    tvi.item.mask = TVIF_TEXT;
    tvi.item.pszText = m_szPath;
    m_hRoot = (HTREEITEM)SendDlgItemMessage(IDC_PKGTREE, TVM_INSERTITEM, 0, (LPARAM)&tvi);

    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, ThreadScan, this, 0, NULL);
    return 0;
}

LRESULT CMainDlg::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (LOWORD(wParam) == ID_APP_ABOUT)
    {
        CComObject<CAboutDlg> dlgAbout;
        dlgAbout.DoModal(m_hWnd);
    }
    else
    {
        bHandled = FALSE;
    }
    return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (NULL != m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
    {
        return MessageBox(CStr::Load(IDS_TITLE), CStr::Load(IDS_TITLE), MB_ICONWARNING);
    }

    CloseDialog(wID);
    return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
    DestroyWindow();
    ::PostQuitMessage(nVal);
}

HRESULT FindRefer(IXMLDOMElement* pRoot, BSTR queryString)
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMNode> pNode = NULL;
    CComPtr<IXMLDOMNamedNodeMap> pMap = NULL;
    CComVariant vbValue;

    hr = pRoot->selectSingleNode(queryString, &pNode);
    if (FAILED(hr) || NULL == pNode) return hr;

    hr = pNode->get_attributes(&pMap);
    if (FAILED(hr) || NULL == pMap) return hr;

    pNode = NULL;
    hr = pMap->getNamedItem(L"name", &pNode);
    if (SUCCEEDED(hr))
    {
        hr = pNode->get_nodeValue(&vbValue);
        CComBSTR(vbValue.bstrVal);
    }
    return hr;
}

DWORD WINAPI CMainDlg::ThreadScan(LPVOID lpParam)
{
    CMainDlg *pDlg = (CMainDlg *)lpParam;
    HRESULT hr = ::CoInitialize(NULL);
    ATLASSERT(SUCCEEDED(hr));

    CComPtr<IXMLDOMDocument> pXml = NULL;
    hr = pXml.CoCreateInstance(_uuidof(DOMDocument));
    if (SUCCEEDED(hr))
    {
        WIN32_FIND_DATA wfData;
        TCHAR szSearch[MAX_PATH], szXml[MAX_PATH];
        ::ZeroMemory(&wfData, sizeof(wfData));
        ::PathCombine(szSearch, pDlg->m_szPath, TEXT("*Package*~*~~*.mum"));

        HANDLE hFind = ::FindFirstFile(szSearch, &wfData);
        if (INVALID_HANDLE_VALUE != hFind)
        {
            CComPtr<IXMLDOMElement> pRoot = NULL;

            do
            {
                VARIANT_BOOL vbLoaded = VARIANT_FALSE;
                ::PathCombine(szXml, pDlg->m_szPath, wfData.cFileName);
                // 加载 mum 文件
                hr = pXml->load(CComVariant(szXml), &vbLoaded);
                if (SUCCEEDED(hr) && vbLoaded == VARIANT_TRUE)
                {
                    hr = pXml->get_documentElement(&pRoot);
                    if (SUCCEEDED(hr) && pRoot != NULL)
                    {
                        FindRefer(pRoot, L"");
                        pRoot = NULL;
                    } 
                }
            } while (::FindNextFile(hFind, &wfData));
            ::FindClose(hFind);
        }
    }
    
    ::CoUninitialize();
    return hr;
}