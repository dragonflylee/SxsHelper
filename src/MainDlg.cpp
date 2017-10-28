#include "StdAfx.h"

UINT CFindDlg::WM_FINDMESSAGE = 0;

CMainDlg::CMainDlg(LPTSTR szPath) :
m_hThread(NULL), m_hPopup(NULL)
{
    ZeroMemory(&m_szExport, sizeof(m_szExport));
    mRoot = new CAssemblyNode();
    mRoot->name = szPath;
}

CMainDlg::~CMainDlg()
{
    
}

LRESULT CMainDlg::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    RECT rc;
    GetClientRect(&rc);
    rc.bottom -= 20;
    mTree.Create(WC_TREEVIEW, m_hWnd, rc, NULL, CWinTraitsOR<TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_CHECKBOXES>::GetWndStyle(0));

    // 设置对话框的图标
    HICON hIcon = ::LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
    SetIcon(hIcon);
    CenterWindow();

    // 加载菜单资源
    m_hPopup = ::LoadMenu(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_POPUP));

    // 设置系统菜单
    HMENU hSysMenu = GetSystemMenu(FALSE);
    CAtlString szAbout;
    szAbout.LoadString(IDS_ABOUT);
    InsertMenu(hSysMenu, 0, MF_SEPARATOR, IDM_ABOUT, NULL);
    InsertMenu(hSysMenu, 0, MF_STRING, IDM_ABOUT, szAbout);

    // 注册查找对话框事件
    CFindDlg::WM_FINDMESSAGE = ::RegisterWindowMessage(FINDMSGSTRING);

    // 从注册表读取默认保存路径
    HKEY hKey = NULL;
    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, 
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), 0, KEY_READ, &hKey))
    {
        DWORD dwSize = sizeof(m_szExport);
        ::RegQueryValueEx(hKey, TEXT("Personal"), NULL, NULL, (PBYTE)m_szExport, &dwSize);
        ::RegCloseKey(hKey);
    }
    ::PathCombine(m_szExport, m_szExport, TEXT("Remove.txt"));

    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    ::PostQuitMessage(LOWORD(wParam));
    return TRUE;
}

LRESULT CMainDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    return mTree.ResizeClient(LOWORD(lParam), HIWORD(lParam));
}

LRESULT CMainDlg::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    switch (LOWORD(wParam))
    {
    case IDM_ABOUT:
    {
        ATL::CSimpleDialog<IDD_ABOUTBOX> dlgAbout;
        return dlgAbout.DoModal(m_hWnd);
    }
    case SC_CLOSE:
        bHandled = IsWorking();
        break;
    }
    bHandled = FALSE;
    return S_OK;
}

/**
 * 超找相关逻辑
 */
LRESULT CMainDlg::OnFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LPFINDREPLACE pfr = (LPFINDREPLACE)lParam;
    if (pfr->Flags & FR_DIALOGTERM)
    {
        mFind.m_hWnd = NULL;
        pfr->Flags &= ~FR_DIALOGTERM;
        return TRUE;
    }

    PTSTR(WINAPI *fnStr)(PCTSTR, PCTSTR) = (pfr->Flags & FR_MATCHCASE) ? ::StrStr : ::StrStrI;
    ULONG(WINAPI *fnInc)(LPLONG) = (pfr->Flags & FR_DOWN) ? CComSingleThreadModel::Increment : CComSingleThreadModel::Decrement;

    LONG index = (LONG)pfr->lCustData;

    if (index >= 0 && index < mMap.GetSize())
    {
        TreeView_SetItemState(mTree, mMap.GetValueAt(index)->Parent.GetValueAt(0), 0, TVIS_BOLD);
    }
    else
    {
        index = 0;
    }

    while (fnInc(&index) < (ULONG)mMap.GetSize() && index >= 0)
    {
        if (fnStr(mMap.GetKeyAt(index), pfr->lpstrFindWhat) != NULL)
        {
            HTREEITEM hItem = mMap.GetValueAt(index)->Parent.GetValueAt(0);
            pfr->lCustData = index;
            TreeView_SetItemState(mTree, hItem, TVIS_BOLD, TVIS_BOLD);
            return TreeView_SelectItem(mTree, hItem);
        }
    }
    return FALSE;
}

LRESULT CMainDlg::OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = mFind.GetNotifier();
    if (pfr->lpstrFindWhat[0] == _T('\0')) return FALSE;

    pfr->Flags |= FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

LRESULT CMainDlg::OnFindPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = mFind.GetNotifier();
    if (pfr->lpstrFindWhat[0] == _T('\0')) return FALSE;

    pfr->Flags &= ~FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

/**
* 树节点级联选中
*/
LRESULT CMainDlg::OnItemChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMTVITEMCHANGE *pChange = (NMTVITEMCHANGE *)pnmh;
    CAssemblyNode *pNode = (CAssemblyNode *)pChange->lParam;
    BOOL bChecked = (pChange->uStateNew >> 12) == 2 ? TRUE : FALSE;
    // 选中状态改变
    if (NULL != pNode && bChecked != pNode->bCheck)
    {
        pNode->bCheck = bChecked;
        for (int i = 0; i < pNode->Parent.GetSize(); i++)
        {
            CAssemblyNode *pParent = pNode->Parent.GetKeyAt(i);
            HTREEITEM hItem = pNode->Parent.GetValueAt(i);
            if (NULL != pParent && ((bChecked ^ pParent->bCheck)))
            {
                BOOL bAll = TRUE;
                for (int j = 0; j < pParent->Package.GetSize(); j++)
                {
                    CAssemblyNode *pChild = pParent->Package.GetValueAt(j);
                    if (NULL != pChild && !pChild->bCheck)
                    {
                        bAll = FALSE;
                        break;
                    }
                }

                if (!bChecked) pParent->bCheck = bAll;

                for (int j = 0; j < pParent->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(pnmh->hwndFrom, pParent->Parent.GetValueAt(j), bAll);
                }
            }
            if (pChange->hItem != hItem)
            {
                TreeView_SetCheckState(pnmh->hwndFrom, hItem, bChecked);
            }
        }
        for (int i = 0; i < pNode->Package.GetSize(); i++)
        {
            CAssemblyNode *pChild = pNode->Package.GetValueAt(i);
            if (NULL != pChild)
            {
                for (int j = 0; j < pChild->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(pnmh->hwndFrom, pChild->Parent.GetValueAt(j), bChecked);
                }
            }
        }
    }
    return 0;
}

LRESULT CMainDlg::OnContext(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    POINT pt = { 0 };
    // 弹出右键菜单
    ::GetCursorPos(&pt);
    return ::TrackPopupMenu(::GetSubMenu(m_hPopup, 0), TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
}

/**
* 递归导出包名列表
*/
void RecurveExport(CAssemblyNode *pParent, HANDLE hFile)
{
    CHAR szName[MAX_PATH * 2];
    for (int i = 0; i < pParent->Package.GetSize(); i++)
    {
        CAssemblyNode *pNode = pParent->Package.GetValueAt(i);
        if (NULL == pNode) continue;

        if (pNode->bCheck)
        {
            int nLen = sprintf_s(szName, _countof(szName), "%ws\r\n", (LPCTSTR)pNode->name);
            ::WriteFile(hFile, szName, nLen, NULL, NULL);
        }
        else if (pNode->Package.GetSize() > 0)
        {
            RecurveExport(pNode, hFile);
        }
    }
}

LRESULT CMainDlg::OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAtlString szTitle, szFilter;
    szTitle.LoadString(IDS_NAMESAVE);
    szFilter.LoadString(IDS_NAMEFILTER);
    szFilter.Replace(TEXT('|'), TEXT('\0'));
   
    OPENFILENAME ofn = { sizeof(OPENFILENAME) };
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrTitle = szTitle;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrDefExt = TEXT("txt");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = m_szExport;

    if (!::GetSaveFileName(&ofn)) return FALSE;

    HANDLE hFile = ::CreateFile(m_szExport, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) return FALSE;

    RecurveExport(mRoot, hFile);
    return ::CloseHandle(hFile);
}

LRESULT CMainDlg::OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // 树为空不能查找
    if (mRoot->Package.GetSize() == 0) return FALSE;
    if (IsWorking()) return FALSE;
    if (NULL != mFind.m_hWnd && mFind.IsWindow()) return TRUE;
    return mFind.Create(FR_DOWN, m_hWnd);
}

LRESULT CMainDlg::OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (IsWorking()) return FALSE;

    TreeView_DeleteAllItems(mTree);
    mRoot->Package.RemoveAll();
    mMap.RemoveAll();
 
    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    return TRUE;
}

BOOL CMainDlg::IsWorking()
{
    if (NULL != m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
    {
        CAtlString szTitle, szText;
        szTitle.LoadString(IDS_TITLE);
        szText.LoadString(IDS_BUSY);
        return MessageBox(szText, szTitle, MB_ICONWARNING);
    }
    return FALSE;
}