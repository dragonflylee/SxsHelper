#pragma once

///////////////////////////////////////////////////////////////////////////////
// CPlacementPersist helper for Window Position persistency

class CPlacementPersist
{
public:
    CPlacementPersist(LPCTSTR sAppKey)
    {
        ATLASSERT(sAppKey && *sAppKey);
        m_Key.Create(HKEY_CURRENT_USER, sAppKey);
        ATLASSERT(m_Key.m_hKey);
    }
    HRESULT Save(HWND hWnd)
    {
        WINDOWPLACEMENT wp = { sizeof(wp) };
        if (!::GetWindowPlacement(hWnd, &wp)) return HRESULT_FROM_WIN32(::GetLastError());

        return HRESULT_FROM_WIN32(m_Key.SetBinaryValue(_T("Placement"), &wp, sizeof(wp)));
    }
    HRESULT Restore(HWND hWnd)
    {
        WINDOWPLACEMENT wp;
        ULONG cbLen = sizeof(wp);
        LONG lRes = m_Key.QueryBinaryValue(_T("Placement"), &wp, &cbLen);
        if (ERROR_SUCCESS != lRes) return HRESULT_FROM_WIN32(lRes);
        ATLASSERT(cbLen == sizeof(wp));

        if (::SetWindowPlacement(hWnd, &wp)) return S_OK;
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    HRESULT Delete()
    {
        return HRESULT_FROM_WIN32(m_Key.DeleteValue(_T("Placement")));
    }
private:
    ATL::CRegKey m_Key;
};
