#include "StdAfx.h"

// ע����ҶԻ����¼�
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

    // ����ϵͳ�˵�
    HMENU hMenu = GetSystemMenu(FALSE);
    BOOL_CHECK(szText.LoadString(IDS_ABOUT));
    BOOL_CHECK(::InsertMenu(hMenu, 0, MF_SEPARATOR, IDM_ABOUT, NULL));
    BOOL_CHECK(::InsertMenu(hMenu, 0, MF_STRING, IDM_ABOUT, szText));

    // ���ô���ͼ��
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

    // ���ز˵���Դ
    m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAIN));
    BOOL_CHECK(m_hMenu);

    BOOL_CHECK(m_tree.Create(m_hWnd, NULL));

    // ���� ɸѡ�� �ؼ�
    BOOL_CHECK(szText.LoadString(IDS_FILTERHINT));
    BOOL_CHECK(m_filter.Create(m_tree, NULL));
    m_filter.SendMessage(EM_SETCUEBANNER, TRUE, (LPARAM)(LPCTSTR)szText);
    m_filter.SendMessage(EM_LIMITTEXT, MAX_PATH);
    m_filter.SetFont(m_hFont);

    // �ӻ�ȡĬ�ϱ���·��
    BOOL_CHECK(::SHGetSpecialFolderPath(m_hWnd, m_szExport, CSIDL_MYDOCUMENTS, TRUE));
   
    // ����ɨ���߳�
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    BOOL_CHECK(m_hThread);
exit:
    // ���� -1 ��ʾ���ڴ���ʧ��
    return SUCCEEDED(hr) ? 0 : -1;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (NULL != m_hFont) ::DeleteObject(m_hFont);
    // �ͷŲ˵���Դ
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
 * ��������߼�
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
    // ��ȡ��ǰ�Ľڵ�
    tvi.hItem = TreeView_GetSelection(m_tree);
    if (NULL != pfr->lCustData) TreeView_SetItemState(m_tree, (HTREEITEM)pfr->lCustData, 0, TVIS_BOLD);

    PTSTR(WINAPI *pStrStr)(PCTSTR, PCTSTR) = (pfr->Flags & FR_MATCHCASE) ? ::StrStr : ::StrStrI;

    // ��ǰ����
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
    // ��ʾû���ҵ�
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
* ���ڵ㼶��ѡ��
*/
LRESULT CMainDlg::OnChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMTVITEMCHANGE *pChange = (NMTVITEMCHANGE *)pnmh;
    CAssemblyNode *pNode = reinterpret_cast<CAssemblyNode *>(pChange->lParam);
    if (NULL == pNode) return S_OK;

    BOOL bChecked = (pChange->uStateNew >> 12) == 2 ? TRUE : FALSE;
    if (bChecked == pNode->bCheck) return S_OK;
    // ���ڵ㼶��ѡ��
    pNode->bCheck = bChecked;
    for (size_t i = 0; i < pNode->Package.GetCount(); i++)
    {
        CAssemblyNode *pChild = pNode->Package.GetAt(i);
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

    for (size_t i = 0; i < pAssembly->Component.GetCount(); i++)
    {
        CAssemblyNode *pNode = pAssembly->Component.GetAt(i);
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
    // �����Ҽ��˵�
    ::GetCursorPos(&pt);
    return ::TrackPopupMenu(::GetSubMenu(m_hMenu, 0), TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
}

LRESULT CMainDlg::OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAtlString szFilter;
    TCHAR szExport[MAX_PATH] = { 0 };
    szFilter.LoadString(IDS_FILTERSAVE);
    szFilter.Replace(TEXT('|'), TEXT('\0'));
    ::PathCombine(szExport, m_szExport, (wID == IDM_EXPORT) ? TEXT("Remove.txt") : TEXT("Dump.txt"));
   
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
        return MessageBox(szFilter, CMainDlg::GetWndCaption(), MB_ICONERROR);
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

LRESULT CMainDlg::OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // ��Ϊ�ղ��ܲ���
    if (mapPackage.GetSize() == 0) return FALSE;
    if (IsWorking()) return FALSE;
    if (NULL != m_find.m_hWnd && m_find.IsWindow()) return TRUE;
    return m_find.Create(FR_DOWN, m_hWnd);
}

LRESULT CMainDlg::OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (IsWorking()) return FALSE;

    // ɾ���ڵ�
    TreeView_DeleteAllItems(m_tree);
    nodeRoot.Package.RemoveAll();
    mapPackage.RemoveAll();

    // ����ɨ���߳�
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
                    HTREEITEM hItem = pNode->Parent.GetValueAt(j);
                    TreeView_SetItemState(m_tree, hItem, TVIS_BOLD, TVIS_BOLD);
                    TreeView_SetCheckState(m_tree, hItem, TRUE);
                    TreeView_EnsureVisible(m_tree, hItem);
                }
            }
            else
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    HTREEITEM hItem = pNode->Parent.GetValueAt(j);
                    TreeView_SetItemState(m_tree, hItem, 0, TVIS_BOLD);
                    TreeView_SetCheckState(m_tree, hItem, FALSE);
                    TreeView_Expand(m_tree, hItem, TVE_COLLAPSE);
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