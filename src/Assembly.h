#ifndef _ASSEMBLY_TREE_H_
#define _ASSEMBLY_TREE_H_

class CAssemblyNode;
typedef ATL::CSimpleMap<CAtlString, CComPtr<CAssemblyNode>> CAssemblyMap;
typedef ATL::CSimpleMap<CAssemblyNode *, HTREEITEM> CAssemblySet;

class CAssemblyNode : public CComObjectRoot, public IUnknown
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

    BEGIN_COM_MAP(CAssemblyNode)
    END_COM_MAP()

public:
    CAssemblyNode() : bCheck(FALSE) {}
};

#endif // _ASSEMBLY_TREE_H_