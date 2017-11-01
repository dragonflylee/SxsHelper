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
    mRoot = new CComObject<CAssemblyNode>();

    ::PathCombine(mRoot->name.GetBufferSetLength(MAX_PATH), szPath, TEXT("Servicing\\Packages"));
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

    int nSize = ::MulDiv(14, ::GetDeviceCaps(GetDC(), LOGPIXELSY), 72);
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
        mFind.m_hWnd = NULL;
        pfr->Flags &= ~FR_DIALOGTERM;
        return TRUE;
    }

    PTSTR(WINAPI *fnStr)(PCTSTR, PCTSTR) = (pfr->Flags & FR_MATCHCASE) ? ::StrStr : ::StrStrI;
    ULONG(WINAPI *fnInc)(LPLONG) = (pfr->Flags & FR_DOWN) ? CComSingleThreadModel::Increment : CComSingleThreadModel::Decrement;

    LONG index = (LONG)pfr->lCustData;
    if (index >= 0 && index < mMap.GetSize())
    {
        TreeView_SetItemState(m_tree, mMap.GetValueAt(index)->Parent.GetValueAt(0), 0, TVIS_BOLD);
    }
    else
    {
        index = 0;
    }

    while (fnInc(&index) < (ULONG)mMap.GetSize() && index >= 0)
    {
        HTREEITEM hItem = mMap.GetValueAt(index)->Parent.GetValueAt(0);

        if (fnStr(mMap.GetKeyAt(index), pfr->lpstrFindWhat) != NULL)
        { 
            pfr->lCustData = index;
            TreeView_SetItemState(m_tree, hItem, TVIS_BOLD, TVIS_BOLD);
            return TreeView_SelectItem(m_tree, hItem);
        }

        TreeView_SetItemState(m_tree, hItem, 0, TVIS_BOLD);
        TreeView_Expand(m_tree, hItem, TVE_COLLAPSE);
    }

    // 提示没有找到
    CAtlString szEnd;
    szEnd.Format(IDS_FINDEND, pfr->lpstrFindWhat);
    return MessageBox(szEnd, CMainDlg::GetWndCaption(), MB_ICONWARNING);
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
LRESULT CMainDlg::OnChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
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

LRESULT CMainDlg::OnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    TVHITTESTINFO ht = { 0 };
    ::GetCursorPos(&ht.pt);
    m_tree.ScreenToClient(&ht.pt);

    if (TreeView_HitTest(m_tree, &ht) && (ht.flags & TVHT_ONITEMLABEL))
    {
        TVITEM tvi = { 0 };
        tvi.hItem = ht.hItem;
        tvi.mask = TVIF_PARAM;
        TreeView_GetItem(m_tree, &tvi);

        MessageBox(((CAssemblyNode*)tvi.lParam)->name);
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
            DWORD cbLen = (DWORD)sprintf_s(szName, _countof(szName), "%ws\r\n", (LPCTSTR)pNode->name);
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

    TreeView_DeleteAllItems(m_tree);
    mRoot->Package.RemoveAll();
    mMap.RemoveAll();
 
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
        for (int i = 0; i < mMap.GetSize(); i++)
        {
            CAssemblyNode *pNode = mMap.GetValueAt(i);
            if (::StrStrI(pNode->name, szFind) != NULL)
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetItemState(m_tree, pNode->Parent.GetValueAt(j), TVIS_BOLD, TVIS_BOLD);
                    TreeView_EnsureVisible(m_tree, pNode->Parent.GetValueAt(j));
                }
            }
            else
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetItemState(m_tree, pNode->Parent.GetValueAt(j), 0, TVIS_BOLD);
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