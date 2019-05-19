#include "SxsHelper.h"
#include "Persist.h"

LPCTSTR lpRegKey = _T("Software\\SxsHelper");

CMainFrm::CMainFrm() : m_hThread(NULL), m_pFind(NULL)
{
}

int CMainFrm::OnCreate(LPCREATESTRUCT /*lpCreate*/)
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
    // 创建视图
    m_hWndClient = wndTree.Create(this, 1, m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | TVS_HASLINES |
        TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT);
    wndTree.SetExtendedStyle(TVS_EX_PARTIALCHECKBOXES, 0);
    wndTree.ModifyStyle(0, TVS_CHECKBOXES);
    // 创建搜索栏
    BOOL_CHECK(wndFilter.Create(this, 2, m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE |
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBS_DROPDOWN | CBS_AUTOHSCROLL));
    BOOL_CHECK(CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE));
    BOOL_CHECK(AddSimpleReBarBand(wndFilter, NULL, TRUE));
    BOOL_CHECK(CreateSimpleStatusBar());
    // 读取之前窗口位置
    if (FAILED(CPlacementPersist(lpRegKey).Restore(m_hWnd))) CenterWindow();

    HMENU hMruMenu = ::GetSubMenu(GetMenu(), 0);
    m_mru.SetMenuHandle(hMruMenu);
    m_mru.ReadFromRegistry(lpRegKey);

    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);

exit:
    // 返回 -1 表示窗口创建失败
    return SUCCEEDED(hr) ? 0 : -1;
}

void CMainFrm::OnDestroy()
{
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);

    if (NULL != m_pFind) m_pFind->SendMessage(WM_CLOSE);

    CPlacementPersist(lpRegKey).Save(m_hWnd);
    ::PostQuitMessage(0);
}

void CMainFrm::OnSysCommand(UINT nID, CPoint /*pt*/)
{
    switch (nID)
    {
    case IDM_ABOUT:
        OnAbout(0, nID, m_hWnd);
        break;
    case SC_CLOSE:
        if (NULL != m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
        {
            CAtlString szText;
            szText.LoadString(IDS_BUSY);
            MessageBox(szText, NULL, MB_ICONWARNING);
            break;
        }
    default:
        SetMsgHandled(FALSE);
    }
}

LRESULT CMainFrm::OnFindCmd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_pFind != NULL)
    {
        if (m_pFind->IsTerminating())
        {
            m_pFind = NULL;
            return S_OK;
        }

        if (m_pFind->FindNext())
        {
            
        }
    }
    return S_OK;
}

void CMainFrm::OnOpen(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
{
    if (m_dlgFolder.DoModal(m_hWnd) == IDOK)
    {
        m_hThread = ::CreateThread(NULL, 0, CMainFrm::ThreadScan, this, 0, NULL);
        m_mru.AddToList(m_dlgFolder.m_szFolderPath);
        m_mru.WriteToRegistry(lpRegKey);
    }
}

void CMainFrm::OnAbout(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
{
    ATL::CSimpleDialog<IDD_ABOUT> dlgAbout;
    dlgAbout.DoModal(m_hWnd);
}

void CMainFrm::OnFind(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
{
    if (m_pFind == NULL)
    {
        m_pFind = new CFindDlg();
        m_pFind->Create(TRUE, NULL, NULL, FR_DOWN, m_hWnd);
    }
    m_pFind->SetActiveWindow();
    m_pFind->ShowWindow(SW_SHOW);
}

void CMainFrm::OnRecent(UINT /*uNotifyCode*/, int nID, HWND /*wndCtl*/)
{
    if (m_mru.GetFromList(nID, m_dlgFolder.m_szFolderPath, _countof(m_dlgFolder.m_szFolderPath)))
    {
        if (!::PathIsDirectory(m_dlgFolder.m_szFolderPath))
        {
            m_mru.RemoveFromList(nID);
            m_mru.WriteToRegistry(lpRegKey);
        }
        else if (NULL == m_hThread)
        {
            m_hThread = ::CreateThread(NULL, 0, CMainFrm::ThreadScan, this, 0, NULL);
            m_mru.MoveToTop(nID);
            m_mru.WriteToRegistry(lpRegKey);
        }
    }
}

LRESULT CMainFrm::OnChanging(LPNMHDR pnmh)
{
    NMTVITEMCHANGE *pnvc = reinterpret_cast<NMTVITEMCHANGE*>(pnmh);
    if (pnvc->lParam == NULL) return TRUE;
    // 选中状态
    UINT uCheck = pnvc->uStateNew >> 12;
    // 父节点
    if ((pnvc->uStateOld >> 12) & ~uCheck)
    {
        HTREEITEM hItem = wndTree.GetChildItem(pnvc->hItem);
        while (hItem != NULL)
        {
            wndTree.SetCheckState(hItem, uCheck - 1);
            hItem = wndTree.GetNextSiblingItem(hItem);
        }
        return FALSE;
    }
    HTREEITEM hItem = wndTree.GetParentItem(pnvc->hItem);
    if (hItem != NULL) wndTree.SetCheckState(hItem, FALSE);
    return TRUE;
}