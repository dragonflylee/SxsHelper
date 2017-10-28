#include "StdAfx.h"

/**
* 查找单个元素
*/
HRESULT FindSingle(IXMLDOMElement* pRoot, BSTR queryString, CAssemblyNode *pAssembly)
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMNode> pNode = NULL;
    CComPtr<IXMLDOMNamedNodeMap> pMap = NULL;
    CComVariant vbValue = NULL;

    hr = pRoot->selectSingleNode(queryString, &pNode);
    if (FAILED(hr) || NULL == pNode) return hr;

    hr = pNode->get_attributes(&pMap);
    if (FAILED(hr) || NULL == pMap) return hr;

    pNode = NULL;
    hr = pMap->getNamedItem(L"name", &pNode);
    if (SUCCEEDED(hr) && SUCCEEDED(pNode->get_nodeValue(&vbValue)))
    {
        pAssembly->name = vbValue.bstrVal;
    }

    pNode = NULL;
    hr = pMap->getNamedItem(L"processorArchitecture", &pNode);
    if (SUCCEEDED(hr) && SUCCEEDED(pNode->get_nodeValue(&vbValue)))
    {
        pAssembly->processorArchitecture = vbValue.bstrVal;
    }

    pNode = NULL;
    hr = pMap->getNamedItem(L"language", &pNode);
    if (SUCCEEDED(hr) && SUCCEEDED(pNode->get_nodeValue(&vbValue)))
    {
        pAssembly->language = vbValue.bstrVal;
    }

    pNode = NULL;
    hr = pMap->getNamedItem(L"version", &pNode);
    if (SUCCEEDED(hr) && SUCCEEDED(pNode->get_nodeValue(&vbValue)))
    {
        pAssembly->version = vbValue.bstrVal;
    }

    pNode = NULL;
    hr = pMap->getNamedItem(L"publicKeyToken", &pNode);
    if (SUCCEEDED(hr) && SUCCEEDED(pNode->get_nodeValue(&vbValue)))
    {
        pAssembly->publicKeyToken = vbValue.bstrVal;
    }
    return hr;
}

/**
* 查找多个元素
*/
HRESULT FindList(IXMLDOMElement* pRoot, BSTR queryString, CAssemblyMap& nodeMap)
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMNode> pNode = NULL;
    CComPtr<IXMLDOMNodeList> pList = NULL;
    CComPtr<IXMLDOMNamedNodeMap> pMap = NULL;
    CComVariant vbValue = NULL;
    long length = 0;

    hr = pRoot->selectNodes(queryString, &pList);
    if (FAILED(hr) || NULL == pList) return hr;

    hr = pList->get_length(&length);
    if (FAILED(hr) || 0 == length) return hr;
    
    for (long index = 0; index < length; index++)
    {
        hr = pList->get_item(index, &pNode);
        if (SUCCEEDED(hr) && NULL != pNode)
        {
            hr = pNode->get_attributes(&pMap);
            if (SUCCEEDED(hr) && NULL != pMap)
            {
                pNode = NULL;
                hr = pMap->getNamedItem(L"name", &pNode);
                if (SUCCEEDED(hr) && SUCCEEDED(pNode->get_nodeValue(&vbValue)))
                {
                    nodeMap.Add(vbValue.bstrVal, NULL);
                }
                pMap = NULL;
            }
            pNode = NULL;
        }
    }
    return hr;
}

/**
* 递归插入TreeView
*/
void CMainDlg::RecurveInsert(HTREEITEM hParent, CAssemblyNode *pParent)
{
    TV_INSERTSTRUCT tvi;
    tvi.hParent = hParent;
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;

    for (int i = 0; i < pParent->Package.GetSize(); i++)
    {
        CAssemblyNode *pNode = pParent->Package.GetValueAt(i);
        if (NULL != pNode)
        {
            tvi.item.pszText = pNode->name.GetBuffer();
            tvi.item.lParam = (LPARAM)pNode;
            HTREEITEM hItem = TreeView_InsertItem(mTree, &tvi);
            pNode->Parent.SetAt(pParent, hItem);
            // 添加到搜索容器
            mMap.Add(pNode->name, pNode);
            if (pNode->Package.GetSize() > 0) RecurveInsert(hItem, pNode);
        }
    }
}

/**
* 扫描线程工作函数
*/
DWORD CALLBACK CMainDlg::ThreadScan(LPVOID lpParam)
{
    CMainDlg *pDlg = (CMainDlg *)lpParam;
    TCHAR szSearch[MAX_PATH], szXml[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    CComPtr<IXMLDOMDocument> pXml = NULL;
    TV_INSERTSTRUCT tvi = { 0 };
    WIN32_FIND_DATA wfd = { 0 };
    CAssemblyMap dict;
    
    HRESULT hr = ::CoInitialize(NULL);
    HR_CHECK(hr);

    // 初始化 XML 对象
    hr = pXml.CoCreateInstance(_uuidof(DOMDocument), NULL, CLSCTX_INPROC_SERVER);
    HR_CHECK(hr);

    // 开始遍历文件
    ::PathCombine(szSearch, pDlg->mRoot->name, TEXT("*Package*~*~~*.mum"));
    hFind = ::FindFirstFile(szSearch, &wfd);
    HANDLE_CHECK(hFind);
             
    do
    {
        VARIANT_BOOL vbLoaded = VARIANT_FALSE;
        ::PathCombine(szXml, pDlg->mRoot->name, wfd.cFileName);
        // 加载 mum 文件
        hr = pXml->load(CComVariant(szXml), &vbLoaded);
        if (SUCCEEDED(hr) && vbLoaded == VARIANT_TRUE)
        {
            CComPtr<IXMLDOMElement> pRoot = NULL;
            hr = pXml->get_documentElement(&pRoot);
            if (SUCCEEDED(hr) && pRoot != NULL)
            {
                CComPtr<CAssemblyNode> pAssembly = new CAssemblyNode();
                if (SUCCEEDED(FindSingle(pRoot, L"//assembly/assemblyIdentity", pAssembly)))
                {
                    FindList(pRoot, L"//assembly/package/parent/assemblyIdentity", pAssembly->Depend);
                    FindList(pRoot, L"//assembly/package/update/package/assemblyIdentity", pAssembly->Package);
                    FindList(pRoot, L"//assembly/package/update/component/assemblyIdentity", pAssembly->Component);
                    FindList(pRoot, L"//assembly/package/update/driver/assemblyIdentity", pAssembly->Driver);
                    dict.Add(pAssembly->name, pAssembly);
                }
            }
        }
    } while (::FindNextFile(hFind, &wfd));

    // 设置封包父子关系
    for (int i = 0; i < dict.GetSize(); i++)
    {
        CAssemblyNode * pParent = dict.GetValueAt(i);
        for (int j = 0; j < pParent->Package.GetSize(); j++)
        {
            CAssemblyNode * pChild = dict.Lookup(pParent->Package.GetKeyAt(j));
            if (NULL != pChild)
            {
                pChild->Parent.Add(pParent, NULL);
                pParent->Package.SetAtIndex(j, pChild->name, pChild);
            }
        }
    }

    // 将顶级封包添加到Root
    for (int i = 0; i < dict.GetSize(); i++)
    {
        CAssemblyNode * pNode = dict.GetValueAt(i);
        if (pNode->Parent.GetSize() == 0)
        {
            pDlg->mRoot->Package.Add(pNode->name, pNode);
            pNode->Parent.Add(pDlg->mRoot, NULL);
        }
    }

    // 插入根节点
    tvi.hInsertAfter = TVI_ROOT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = pDlg->mRoot->name.GetBuffer();
    tvi.item.lParam = (LPARAM)(CAssemblyNode *)pDlg->mRoot;

    HTREEITEM hRoot = TreeView_InsertItem(pDlg->mTree, &tvi);
    pDlg->RecurveInsert(hRoot, pDlg->mRoot);

    TreeView_SetItemState(pDlg->mTree, hRoot, 0, TVIS_STATEIMAGEMASK);
    TreeView_Expand(pDlg->mTree, hRoot, TVE_EXPAND);

exit:
    if (INVALID_HANDLE_VALUE != hFind) ::FindClose(hFind);

    ::CoUninitialize();
    pDlg->m_hThread = NULL;
    return hr;
}