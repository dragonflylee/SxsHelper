#include "SxsHelper.h"

// 注册查找对话框事件
UINT CFindDlg::WM_FIND = ::RegisterWindowMessage(FINDMSGSTRING);

CMainFrm::CMainFrm(LPCTSTR szPath) :
m_hThread(NULL),
m_hMenu(NULL),
m_hFont(NULL)
{
    ZeroMemory(&szExport, sizeof(szExport));
    nodeRoot.szName = szPath;
}

LPCTSTR CMainFrm::GetWndCaption()
{
    static CAtlString szTitle;
    if (szTitle.IsEmpty()) szTitle.LoadString(IDR_MAIN);
    return szTitle;
}

LRESULT CMainFrm::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HRESULT hr = S_OK;
    CAtlString szText;

    // 设置系统菜单
    HMENU hMenu = GetSystemMenu(FALSE);
    BOOL_CHECK(szText.LoadString(IDS_ABOUT));
    BOOL_CHECK(::InsertMenu(hMenu, 0, MF_SEPARATOR, IDM_ABOUT, NULL));
    BOOL_CHECK(::InsertMenu(hMenu, 0, MF_STRING, IDM_ABOUT, szText));

    // 设置窗体图标
    SetIcon(::LoadIcon(_Module.m_hInst, MAKEINTRESOURCE(IDR_MAIN)));
    BOOL_CHECK(CenterWindow());

    HDC hDC = GetDC();
    int nSize = ::MulDiv(16, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
    ReleaseDC(hDC);
    m_hFont = ::CreateFont(nSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("MS Shell Dlg"));
    BOOL_CHECK(m_hFont);

    // 加载菜单资源
    m_hMenu = ::LoadMenu(_Module.m_hInst, MAKEINTRESOURCE(IDR_MAIN));
    BOOL_CHECK(m_hMenu);

    BOOL_CHECK(wndTree.Create(WC_TREEVIEW, this, 1, m_hWnd, NULL));

    // 创建 筛选器 控件
    BOOL_CHECK(szText.LoadString(IDS_FILTERHINT));
    BOOL_CHECK(wndFilter.Create(WC_EDIT, this, 2, wndTree, NULL));
    wndFilter.SendMessage(EM_SETCUEBANNER, TRUE, (LPARAM)szText.GetString());
    wndFilter.SendMessage(EM_LIMITTEXT, MAX_PATH);
    wndFilter.SetFont(m_hFont);

    // 从获取默认保存路径
    BOOL_CHECK(::SHGetSpecialFolderPath(m_hWnd, szExport, CSIDL_MYDOCUMENTS, TRUE));
    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, CMainFrm::ThreadScan, this, 0, NULL);
    BOOL_CHECK(m_hThread);
exit:
    // 返回 -1 表示窗口创建失败
    return SUCCEEDED(hr) ? 0 : -1;
}

void CMainFrm::OnFinalMessage(HWND /*hWnd*/)
{
    if (NULL != m_hFont) ::DeleteObject(m_hFont);
    // 释放菜单资源
    if (NULL != m_hMenu) ::DestroyMenu(m_hMenu);
    ::PostQuitMessage(0);
}

LRESULT CMainFrm::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    wndTree.ResizeClient(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return wndFilter.SetWindowPos(HWND_TOP, GET_X_LPARAM(lParam) - 180, 0, 180, 25, 0);
}

LRESULT CMainFrm::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    switch (LOWORD(wParam))
    {
    case IDM_ABOUT:
    {
        ATL::CSimpleDialog<IDD_ABOUT> dlgAbout;
        return dlgAbout.DoModal(m_hWnd);
    }
    case SC_CLOSE:
        bHandled = IsWorking();
        break;
    default:
        bHandled = FALSE;
    }
    return S_OK;
}

/**
 * 查找相关逻辑
 */
LRESULT CMainFrm::OnFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LPFINDREPLACE pfr = (LPFINDREPLACE)lParam;
    if (pfr->Flags & FR_DIALOGTERM)
    {
        dlgFind.m_hWnd = NULL;
        pfr->Flags &= ~FR_DIALOGTERM;
        return TRUE;
    }

    TCHAR szText[MAX_PATH] = { 0 };
    TVITEM tvi = { TVIF_TEXT };
    tvi.pszText = szText;
    tvi.cchTextMax = _countof(szText);
    // 获取当前的节点
    tvi.hItem = TreeView_GetSelection(wndTree);
    if (NULL != pfr->lCustData) TreeView_SetItemState(wndTree, (HTREEITEM)pfr->lCustData, 0, TVIS_BOLD);

    PTSTR(WINAPI *pStrStr)(PCTSTR, PCTSTR) = (pfr->Flags & FR_MATCHCASE) ? ::StrStr : ::StrStrI;

    // 向前搜索
    if (NULL == tvi.hItem) tvi.hItem = TreeView_GetRoot(wndTree);
    do
    {
        pfr->lCustData = (LPARAM)TreeView_GetChild(wndTree, tvi.hItem);
        while (NULL == pfr->lCustData && NULL != tvi.hItem)
        {
            pfr->lCustData = (LPARAM)TreeView_GetNextSibling(wndTree, tvi.hItem);
            if (NULL == pfr->lCustData) tvi.hItem = TreeView_GetParent(wndTree, tvi.hItem);
        }
        if (NULL == pfr->lCustData) break;
        tvi.hItem = (HTREEITEM)pfr->lCustData;
        TreeView_GetItem(wndTree, &tvi);
    } while (pStrStr(szText, pfr->lpstrFindWhat) == NULL);

    if (NULL != pfr->lCustData)
    {
        TreeView_SetItemState(wndTree, tvi.hItem, TVIS_BOLD, TVIS_BOLD);
        return TreeView_SelectItem(wndTree, tvi.hItem);
    }
    // 提示没有找到
    CAtlString szEnd;
    szEnd.Format(IDS_FINDEND, pfr->lpstrFindWhat);
    return dlgFind.MessageBox(szEnd, CMainFrm::GetWndCaption(), MB_ICONWARNING);
}

LRESULT CMainFrm::OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = dlgFind.GetNotifier();
    if (pfr->lpstrFindWhat[0] == TEXT('\0')) return FALSE;

    pfr->Flags |= FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

LRESULT CMainFrm::OnFindPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = dlgFind.GetNotifier();
    if (pfr->lpstrFindWhat[0] == TEXT('\0')) return FALSE;

    pfr->Flags &= ~FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

/**
* 树节点级联选中
*/
LRESULT CMainFrm::OnChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMTVITEMCHANGE *pChange = (NMTVITEMCHANGE *)pnmh;
    CAssemblyNode *pNode = reinterpret_cast<CAssemblyNode *>(pChange->lParam);
    if (NULL == pNode) return S_OK;

    BOOL bChecked = (pChange->uStateNew >> 12) == 2 ? TRUE : FALSE;
    if (bChecked == pNode->bCheck) return S_OK;
    // 树节点级联选中
    pNode->bCheck = bChecked;
    for (int i = 0; i < pNode->Package.GetSize(); i++)
    {
        CAssemblyNode *pChild = pNode->Package.GetValueAt(i);
        for (int j = 0; j < pChild->Parent.GetSize(); j++)
        {
            TreeView_SetCheckState(pnmh->hwndFrom, pChild->Parent.GetValueAt(j), bChecked);
        }
    }
    return S_OK;
}

LRESULT CMainFrm::OnClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    TVHITTESTINFO ht = { { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) } };
    wndTree.ScreenToClient(&ht.pt);

    if (!TreeView_HitTest(wndTree, &ht) || (ht.flags & TVHT_ONITEMLABEL) == 0) return S_OK;

    TVITEM tvi = { 0 };
    tvi.hItem = ht.hItem;
    tvi.mask = TVIF_PARAM;
    if (!TreeView_GetItem(wndTree, &tvi)) return S_OK;

    TCHAR szWinSxS[MAX_PATH];
    ::PathCombine(szWinSxS, nodeRoot.szName, TEXT("WinSxS\\Manifests"));
    return S_OK;
}

LRESULT CMainFrm::OnContext(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    return ::TrackPopupMenu(::GetSubMenu(m_hMenu, 0), TPM_LEFTALIGN | TPM_LEFTBUTTON, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, m_hWnd, NULL);
}

LRESULT CMainFrm::OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAtlString szFilter;
    TCHAR szExport[MAX_PATH] = { 0 };
    szFilter.LoadString(IDS_FILTERSAVE);
    szFilter.Replace(TEXT('|'), TEXT('\0'));
    ::PathCombine(szExport, szExport, (wID == IDM_EXPORT) ? TEXT("Remove.txt") : TEXT("Dump.txt"));
   
    OPENFILENAME ofn = { sizeof(OPENFILENAME) };
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrDefExt = TEXT("txt");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = szExport;

    if (!::GetSaveFileName(&ofn)) return FALSE;

    HANDLE hFile = ::CreateFile(szExport, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        szFilter.Format(IDS_ERROR, ::GetLastError());
        return MessageBox(szFilter, CMainFrm::GetWndCaption(), MB_ICONERROR);
    }

    CHAR szName[MAX_PATH + 2];
    for (int i = 0; i < mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pNode = mapPackage.GetValueAt(i);
        if (pNode->bCheck)
        {
            BOOL bCheck = TRUE;
            for (int j = 0; j < pNode->Parent.GetSize(); j++) if (pNode->Parent.GetKeyAt(j)->bCheck) bCheck = FALSE;
           // for (size_t j = 0; j < pNode->Depend.GetCount(); j++) if (pNode->Depend.GetAt(j)->bCheck) bCheck = FALSE;
            if (bCheck || wID == IDM_DUMP)
            {
                DWORD cbLen = (DWORD)sprintf_s(szName, _countof(szName), "%ws\r\n", (LPCTSTR)pNode->szName);
                ::WriteFile(hFile, szName, cbLen, &cbLen, NULL);
            }
        }
    }
   
    return ::CloseHandle(hFile);
}

LRESULT CMainFrm::OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // 树为空不能查找
    if (mapPackage.GetSize() == 0) return FALSE;
    if (IsWorking()) return FALSE;
    if (NULL != dlgFind.m_hWnd && dlgFind.IsWindow()) return TRUE;
    return dlgFind.Create(FR_DOWN, m_hWnd);
}

LRESULT CMainFrm::OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (IsWorking()) return FALSE;

    // 删除节点
    TreeView_DeleteAllItems(wndTree);
    nodeRoot.Package.RemoveAll();
    mapPackage.RemoveAll();
    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, CMainFrm::ThreadScan, this, 0, NULL);
    return TRUE;
}

LRESULT CMainFrm::OnFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (VK_RETURN == wParam)
    {
        CAtlString szFind;
        wndFilter.GetWindowText(szFind);
        for (int i = 0; i < mapPackage.GetSize(); i++)
        {
            CAssemblyNode *pNode = mapPackage.GetValueAt(i);
            if (::StrStrI(pNode->szName, szFind) != NULL)
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(wndTree, pNode->Parent.GetValueAt(j), TRUE);
                    TreeView_EnsureVisible(wndTree, pNode->Parent.GetValueAt(j));
                }
            }
            else
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(wndTree, pNode->Parent.GetValueAt(j), FALSE);
                    TreeView_Expand(wndTree, pNode->Parent.GetValueAt(j), TVE_COLLAPSE);
                }
            }
        }
        return TRUE;
    }
    bHandled = FALSE;
    return TRUE;
}

BOOL CMainFrm::IsWorking()
{
    if (NULL != m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
    {
        CAtlString szText;
        szText.LoadString(IDS_BUSY);
        return MessageBox(szText, CMainFrm::GetWndCaption(), MB_ICONWARNING);
    }
    return FALSE;
}