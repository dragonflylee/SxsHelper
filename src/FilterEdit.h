#ifndef _FILTER_EDIT_H_
#define _FILTER_EDIT_H_

class CFilterEdit : public CWindowImpl<CFilterEdit>
{
public:
    DECLARE_WND_SUPERCLASS(TEXT("CFilterEdit"), WC_EDIT)

    BEGIN_MSG_MAP(CFilterEdit)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
    END_MSG_MAP();

public:
    LRESULT OnChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return TRUE;
    }
};

#endif // _FILTER_EDIT_H_
