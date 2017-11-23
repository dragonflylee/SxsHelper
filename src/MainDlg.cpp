#include "StdAfx.h"

// 注册查找对话框事件
UINT CFindDlg::WM_FINDMESSAGE = ::RegisterWindowMessage(FINDMSGSTRING);

CMainDlg::CMainDlg(LPCTSTR szPath) :
m_hThread(NULL),
m_hMenu(NULL),
m_hFont(NULL),
m_tree(WC_TREEVIEW, this, 1),
m_filter(WC_EDIT, this, 2)
{
    ZeroMemory(&m_szExport, sizeof(m_szExport));
    nodeRoot.szName = szPath;
}

LPCTSTR CMainDlg::GetWndCaption()
{
    static CAtlString szTitle;
    if (szTitle.IsEmpty()) szTitle.LoadString(IDR_MAIN);
    return szTitle;
}

LRESULT CMainDlg::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HRESULT hr = S_OK;
    CAtlString szText;

    // 设置系统菜单
    HMENU hMenu = GetSystemMenu(FALSE);
    BOOL_CHECK(szText.LoadString(IDS_ABOUT));
    BOOL_CHECK(::InsertMenu(hMenu, 0, MF_SEPARATOR, IDM_ABOUT, NULL));
    BOOL_CHECK(::InsertMenu(hMenu, 0, MF_STRING, IDM_ABOUT, szText));

    // 设置窗体图标
    HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAIN));
    BOOL_CHECK(hIcon);
    SetIcon(hIcon);
    BOOL_CHECK(CenterWindow());

    HDC hDC = GetDC();
    int nSize = ::MulDiv(14, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
    ReleaseDC(hDC);
    m_hFont = ::CreateFont(nSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("SimHei"));
    BOOL_CHECK(m_hFont);

    // 加载菜单资源
    m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAIN));
    BOOL_CHECK(m_hMenu);

    BOOL_CHECK(m_tree.Create(m_hWnd, NULL));

    // 创建 筛选器 控件
    BOOL_CHECK(szText.LoadString(IDS_FILTERHINT));
    BOOL_CHECK(m_filter.Create(m_tree, NULL));
    m_filter.SendMessage(EM_SETCUEBANNER, TRUE, (LPARAM)(LPCTSTR)szText);
    m_filter.SendMessage(EM_LIMITTEXT, MAX_PATH);
    m_filter.SetFont(m_hFont);

    // 从获取默认保存路径
    BOOL_CHECK(::SHGetSpecialFolderPath(m_hWnd, m_szExport, CSIDL_MYDOCUMENTS, TRUE));
    BOOL_CHECK(::PathCombine(m_szExport, m_szExport, TEXT("Remove.txt")));

    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    BOOL_CHECK(m_hThread);
exit:
    // 返回 -1 表示窗口创建失败
    return SUCCEEDED(hr) ? 0 : -1;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (NULL != m_hFont) ::DeleteObject(m_hFont);
    // 释放菜单资源
    if (NULL != m_hMenu) ::DestroyMenu(m_hMenu);
    ::PostQuitMessage(LOWORD(wParam));
    return S_OK;
}

LRESULT CMainDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    m_tree.ResizeClient(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return m_filter.SetWindowPos(HWND_TOP, GET_X_LPARAM(lParam) - 180, 0, 180, 25, 0);
}

LRESULT CMainDlg::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
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
    }
    bHandled = FALSE;
    return S_OK;
}

/**
 * 查找相关逻辑
 */
LRESULT CMainDlg::OnFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LPFINDREPLACE pfr = (LPFINDREPLACE)lParam;
    if (pfr->Flags & FR_DIALOGTERM)
    {
        m_find.m_hWnd = NULL;
        pfr->Flags &= ~FR_DIALOGTERM;
        return TRUE;
    }

    TCHAR szText[MAX_PATH] = { 0 };
    TVITEM tvi = { TVIF_TEXT };
    tvi.pszText = szText;
    tvi.cchTextMax = _countof(szText);
    // 获取当前的节点
    tvi.hItem = TreeView_GetSelection(m_tree);
    if (NULL != pfr->lCustData) TreeView_SetItemState(m_tree, (HTREEITEM)pfr->lCustData, 0, TVIS_BOLD);

    PTSTR(WINAPI *pStrStr)(PCTSTR, PCTSTR) = (pfr->Flags & FR_MATCHCASE) ? ::StrStr : ::StrStrI;

    // 向前搜索
    if (NULL == tvi.hItem) tvi.hItem = TreeView_GetRoot(m_tree);
    do
    {
        pfr->lCustData = (LPARAM)TreeView_GetChild(m_tree, tvi.hItem);
        while (NULL == pfr->lCustData && NULL != tvi.hItem)
        {
            pfr->lCustData = (LPARAM)TreeView_GetNextSibling(m_tree, tvi.hItem);
            if (NULL == pfr->lCustData) tvi.hItem = TreeView_GetParent(m_tree, tvi.hItem);
        }
        if (NULL == pfr->lCustData) break;
        tvi.hItem = (HTREEITEM)pfr->lCustData;
        TreeView_GetItem(m_tree, &tvi);
    } while (pStrStr(szText, pfr->lpstrFindWhat) == NULL);

    if (NULL != pfr->lCustData)
    {
        TreeView_SetItemState(m_tree, tvi.hItem, TVIS_BOLD, TVIS_BOLD);
        return TreeView_SelectItem(m_tree, tvi.hItem);
    }
    // 提示没有找到
    CAtlString szEnd;
    szEnd.Format(IDS_FINDEND, pfr->lpstrFindWhat);
    return m_find.MessageBox(szEnd, CMainDlg::GetWndCaption(), MB_ICONWARNING);
}

LRESULT CMainDlg::OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = m_find.GetNotifier();
    if (pfr->lpstrFindWhat[0] == TEXT('\0')) return FALSE;

    pfr->Flags |= FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

LRESULT CMainDlg::OnFindPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = m_find.GetNotifier();
    if (pfr->lpstrFindWhat[0] == TEXT('\0')) return FALSE;

    pfr->Flags &= ~FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

/**
* 树节点级联选中
*/
LRESULT CMainDlg::OnChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
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

LRESULT CMainDlg::OnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    TVHITTESTINFO ht = { 0 };
    ::GetCursorPos(&ht.pt);
    m_tree.ScreenToClient(&ht.pt);

    if (!TreeView_HitTest(m_tree, &ht) || (ht.flags & TVHT_ONITEMLABEL) == 0) return S_OK;

    TVITEM tvi = { 0 };
    tvi.hItem = ht.hItem;
    tvi.mask = TVIF_PARAM;
    if (!TreeView_GetItem(m_tree, &tvi)) return S_OK;

    TCHAR szWinSxS[MAX_PATH], szSearch[MAX_PATH] = { 0 };
    ::PathCombine(szWinSxS, nodeRoot.szName, TEXT("WinSxS\\Manifests"));
    CAssemblyNode *pAssembly = reinterpret_cast<CAssemblyNode *>(tvi.lParam);

    for (int i = 0; i < pAssembly->Component.GetSize(); i++)
    {
        CAssemblyNode *pNode = pAssembly->Component.GetValueAt(i);
        CAtlString szPackage = pNode->szName.GetLength() < 40 ? pNode->szName : pNode->szName.Left(19) + TEXT("..") + pNode->szName.Right(19);
        _stprintf_s(szSearch, _countof(szSearch), TEXT("%s\\%s_%s_%s_%s_*.manifest"), szWinSxS,
            (PCTSTR)pNode->szArch, (PCTSTR)szPackage.MakeLower(), (PCTSTR)pNode->szToken, (PCTSTR)pNode->szVersion);

        WIN32_FIND_DATA wfd = { 0 };
        HANDLE hFind = ::FindFirstFile(szSearch, &wfd);
        if (INVALID_HANDLE_VALUE != hFind)
        {
            do
            {
                ATLTRACE(TEXT("%s\n"), wfd.cFileName);
            } while (::FindNextFile(hFind, &wfd));
            ::FindClose(hFind);
        }
    }

    return S_OK;
}

LRESULT CMainDlg::OnContext(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    POINT pt = { 0, 0 };
    // 弹出右键菜单
    ::GetCursorPos(&pt);
    return ::TrackPopupMenu(::GetSubMenu(m_hMenu, 0), TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
}

/**
* 递归导出包名列表
*/
void RecurveExport(CAssemblyNode *pParent, HANDLE hFile)
{
    CHAR szName[MAX_PATH + 2];
    for (int i = 0; i < pParent->Package.GetSize(); i++)
    {
        CAssemblyNode *pNode = pParent->Package.GetValueAt(i);
        if (NULL == pNode) continue;

        if (pNode->bCheck)
        {
            DWORD cbLen = (DWORD)sprintf_s(szName, _countof(szName), "%ws\r\n", (LPCTSTR)pNode->szName);
            ::WriteFile(hFile, szName, cbLen, &cbLen, NULL);
        }
        else if (pNode->Package.GetSize() > 0)
        {
            RecurveExport(pNode, hFile);
        }
    }
}

LRESULT CMainDlg::OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAtlString szFilter;
    szFilter.LoadString(IDS_FILTERSAVE);
    szFilter.Replace(TEXT('|'), TEXT('\0'));

    OPENFILENAME ofn = { sizeof(OPENFILENAME) };
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrDefExt = TEXT("txt");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = m_szExport;

    if (!::GetSaveFileName(&ofn)) return FALSE;

    HANDLE hFile = ::CreateFile(m_szExport, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        szFilter.Format(IDS_ERROR, ::GetLastError());
        return MessageBox(szFilter, CMainDlg::GetWndCaption(), MB_ICONERROR);
    }

    RecurveExport(&nodeRoot, hFile);
    return ::CloseHandle(hFile);
}

LRESULT CMainDlg::OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // 树为空不能查找
    if (mapPackage.GetSize() == 0) return FALSE;
    if (IsWorking()) return FALSE;
    if (NULL != m_find.m_hWnd && m_find.IsWindow()) return TRUE;
    return m_find.Create(FR_DOWN, m_hWnd);
}

LRESULT CMainDlg::OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (IsWorking()) return FALSE;

    // 删除节点
    TreeView_DeleteAllItems(m_tree);
    nodeRoot.Package.RemoveAll();
    mapPackage.RemoveAll();

    // 创建扫描线程
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    return TRUE;
}

LRESULT CMainDlg::OnFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (VK_RETURN == wParam)
    {
        CAtlString szFind;
        m_filter.GetWindowText(szFind);
        for (int i = 0; i < mapPackage.GetSize(); i++)
        {
            CAssemblyNode *pNode = mapPackage.GetValueAt(i);
            if (::StrStrI(pNode->szName, szFind) != NULL)
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(m_tree, pNode->Parent.GetValueAt(j), TRUE);
                    TreeView_EnsureVisible(m_tree, pNode->Parent.GetValueAt(j));
                }
            }
            else
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(m_tree, pNode->Parent.GetValueAt(j), FALSE);
                    TreeView_Expand(m_tree, pNode->Parent.GetValueAt(j), TVE_COLLAPSE);
                }
            }
        }
        return TRUE;
    }
    bHandled = FALSE;
    return TRUE;
}

BOOL CMainDlg::IsWorking()
{
    if (NULL != m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
    {
        CAtlString szText;
        szText.LoadString(IDS_BUSY);
        return MessageBox(szText, CMainDlg::GetWndCaption(), MB_ICONWARNING);
    }
    return FALSE;
}