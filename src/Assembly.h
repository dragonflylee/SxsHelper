#ifndef _ASSEMBLY_TREE_H_
#define _ASSEMBLY_TREE_H_

class CAssemblyNode;
typedef ATL::CSimpleMap<CAtlString, CComPtr<CAssemblyNode>> CAssemblyMap;
typedef ATL::CSimpleMap<CAssemblyNode *, HTREEITEM> CAssemblySet;

class CAssemblyNode : public IUnknown
{
public:
    CAtlString name;
    CAtlString publicKeyToken;
    CAtlString processorArchitecture;
    CAtlString language;
    CAtlString version;

    BOOL bCheck;

    CAssemblySet Parent;
    CAssemblyMap Depend;
    CAssemblyMap Package;
    CAssemblyMap Component;
    CAssemblyMap Driver;

public:
    STDMETHOD(QueryInterface)(REFIID /*riid*/, void ** /*ppv*/) { return E_NOTIMPL; }
    STDMETHOD_(ULONG, AddRef)() { return ::InterlockedIncrement(&m_lRes); }
    STDMETHOD_(ULONG, Release)() { LONG cRef = ::InterlockedDecrement(&m_lRes); if (cRef <= 0) delete this; return cRef; }

private:
    LONG m_lRes;

public:
    CAssemblyNode() : m_lRes(0), bCheck(FALSE) {}
    virtual ~CAssemblyNode()
    {
        ATLTRACE(_T("%s\n"), name);
    }
};

#endif // _ASSEMBLY_TREE_H_