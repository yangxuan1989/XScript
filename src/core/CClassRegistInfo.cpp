﻿#include "core/CCallBase.h"
#include "core/CScriptBase.h"
#include "core/CClassRegistInfo.h"

namespace Gamma
{

    //=====================================================================
    // 类型的继承关系
    //=====================================================================
	CClassRegistInfo::CClassRegistInfo( CScriptBase* pScriptBase, 
		const char* szClassName, const char* szTypeIDName,
		uint32 nSize, MakeTypeFunction funMakeType )
		: CClassName( szClassName )
		, CTypeIDName( szTypeIDName )
		, m_VMObjVTableInfo( NULL, INVALID_32BITID )
		, m_pScriptBase( pScriptBase )
        , m_nSizeOfClass( nSize )
        , m_pObjectConstruct( NULL )
        , m_bIsCallBack(false)
		, m_nInheritDepth(0)
		, m_funMakeType( funMakeType )
		, m_nInstanceCount( 0 )
    {
		// 类的名字太长会导致CLuaObject::GetFromVM函数里面堆栈越界
		assert( m_szClassName.size() < 240 );
		char szBuffer[1024];
		strcpy2array_safe( szBuffer, m_szClassName.c_str() );
		strcat2array_safe( szBuffer, "_hObject" );
		m_strObjectIndex = szBuffer;
    }

    CClassRegistInfo::~CClassRegistInfo()
	{
		for( map<string,CCallBase*>::iterator it = m_mapRegistFunction.begin(); 
			it != m_mapRegistFunction.end(); ++it )
			delete it->second;
    }

	void CClassRegistInfo::SetObjectConstruct( IObjectConstruct* pObjectConstruct )
	{
		 m_pObjectConstruct = pObjectConstruct;
	}

    void CClassRegistInfo::InitVirtualTable( SFunctionTable* pNewTable )
	{
		for( uint32 i = 0; i < m_vecNewFunction.size(); i++ )
		{
			if( !m_vecNewFunction[i] )
				continue;
			CCallScriptBase* pCallInfo = m_vecNewFunction[i];
			assert( pCallInfo->GetFunIndex() == i );
			pNewTable->m_pFun[i] = pCallInfo->GetBootFun();
		}
    }

    int32 CClassRegistInfo::GetMaxRegisterFunctionIndex()
    {        
		return (int32)m_vecNewFunction.size();
    }

    void CClassRegistInfo::Create( void* pObject )
    {
		//声明性质的类不可创建
		assert( m_nSizeOfClass );
		assert( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->Construct( pObject );;
		m_pScriptBase->CheckUnlinkCppObj();
	}

	void CClassRegistInfo::Assign( void* pDest, void* pSrc )
	{
		assert( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->Assign( pDest, pSrc );
	}

    void CClassRegistInfo::Release( void* pObject )
	{
		//声明性质的类不可销毁
		assert( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->Destruct( pObject );
		m_pScriptBase->CheckUnlinkCppObj();
	}

	CTypeBase* CClassRegistInfo::MakeType( bool bValue )
	{
		assert( m_funMakeType );
		return m_funMakeType( this, bValue );
	}

	void CClassRegistInfo::RegistFunction( const string& szFunName, CCallBase* pCallBase )
	{
		assert( m_mapRegistFunction.find( szFunName ) == m_mapRegistFunction.end() );
		m_mapRegistFunction[szFunName] = pCallBase;
	}

	CCallBase* CClassRegistInfo::GetCallBase( const string& strFunName )
	{
		map<string,CCallBase*>::iterator it = m_mapRegistFunction.find( strFunName );
		return it == m_mapRegistFunction.end() ? NULL : it->second;
	}

    void CClassRegistInfo::RegistClassCallBack( uint32 nIndex, CCallScriptBase* pCallScriptBase )
	{
		// 不能重复注册
		if( nIndex >= m_vecNewFunction.size() )
			m_vecNewFunction.resize( nIndex + 1 );
		assert( m_vecNewFunction[nIndex] == NULL );
		m_vecNewFunction[nIndex] = pCallScriptBase;

		for( size_t i = 0; i < m_vecChildRegist.size(); ++i )
		{
			if( m_vecChildRegist[i].m_nBaseOff )
				continue;
			m_vecChildRegist[i].m_pBaseInfo->RegistClassCallBack( nIndex, pCallScriptBase );
		}
	}

    bool CClassRegistInfo::IsCallBack()
    {
		return !m_vecNewFunction.empty();
    }

    void CClassRegistInfo::AddBaseRegist( CClassRegistInfo* pRegist, ptrdiff_t nOffset ) 
    { 
        if( ! pRegist )
            return;
		assert( nOffset >= 0 );
        SBaseInfo BaseInfo = { pRegist, (int32)nOffset };
		if( pRegist->m_nInheritDepth + 1 > m_nInheritDepth )
			m_nInheritDepth = pRegist->m_nInheritDepth + 1;
        m_vecBaseRegist.push_back( BaseInfo );

		BaseInfo.m_pBaseInfo = this;
		BaseInfo.m_nBaseOff = -BaseInfo.m_nBaseOff;
		pRegist->m_vecChildRegist.push_back( BaseInfo );

		if( nOffset )
			return;

		// 自然继承，虚表要延续
		vector<CCallScriptBase*>& vecNewFunction = pRegist->m_vecNewFunction;
		for( uint32 i = 0; i < vecNewFunction.size(); i++ )
		{
			if( !vecNewFunction[i] )
				continue;
			assert( vecNewFunction[i]->GetFunIndex() == i );
			RegistClassCallBack( i, vecNewFunction[i] );
		}
    }

    int32 CClassRegistInfo::GetBaseOffset( CClassRegistInfo* pRegist )
    {
        if( pRegist == this )
            return 0;

        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
        {
            int32 nOffset = m_vecBaseRegist[i].m_pBaseInfo->GetBaseOffset( pRegist );
            if( nOffset >= 0 )
                return m_vecBaseRegist[i].m_nBaseOff + nOffset;
        }

        return -1;
	}

    void CClassRegistInfo::ReplaceVirtualTable( void* pObj, bool bNewByVM, uint32 nInheritDepth )
    {
        SVirtualObj* pVObj        = (SVirtualObj*)pObj;
        SFunctionTable* pOldTable = pVObj->m_pTable;
        SFunctionTable* pNewTable = NULL;

		if( !m_vecNewFunction.empty() )
		{
			// 确保pOldTable是原始虚表，因为pVObj有可能已经被修改过了
			pOldTable = m_pScriptBase->GetOrgVirtualTable( pVObj );
			pNewTable = m_pScriptBase->CheckNewVirtualTable( pOldTable, this, bNewByVM, nInheritDepth );
		}

        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
        {
            if( m_vecBaseRegist[i].m_pBaseInfo->IsCallBack() )
			{
				void* pBaseObj = ( (char*)pObj ) + m_vecBaseRegist[i].m_nBaseOff;
                m_vecBaseRegist[i].m_pBaseInfo->ReplaceVirtualTable( pBaseObj, bNewByVM, nInheritDepth + 1 );
			}
        }

        if( pNewTable )
		{
			// 不允许不同的虚拟机共同使用同一份虚表
			assert( !CScriptBase::IsAllocVirtualTable( pVObj->m_pTable ) ||
				( (CClassRegistInfo*)( pVObj->m_pTable->m_pFun[-1] ) )->m_pScriptBase == m_pScriptBase );
            pVObj->m_pTable = pNewTable;
		}
    }

    void CClassRegistInfo::RecoverVirtualTable( void* pObj )
    {
        SFunctionTable* pOrgTable = NULL;
        if( !m_vecNewFunction.empty() )
            pOrgTable = m_pScriptBase->GetOrgVirtualTable( pObj );

        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
            m_vecBaseRegist[i].m_pBaseInfo->RecoverVirtualTable( ( (char*)pObj ) + m_vecBaseRegist[i].m_nBaseOff );

        if( pOrgTable )
            ( (SVirtualObj*)pObj )->m_pTable = pOrgTable;
    }

    bool CClassRegistInfo::FindBase( CClassRegistInfo* pRegistBase )
    {
        if( pRegistBase == this )
            return true;
        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
            if( m_vecBaseRegist[i].m_pBaseInfo->FindBase( pRegistBase ) )
                return true;
        return false;
    }

	bool CClassRegistInfo::IsBaseObject( ptrdiff_t nDiff )
	{
		// 命中基类
		if( nDiff == 0 )
			return true;

		// 超出基类内存范围
		if( nDiff > (int32)GetClassSize() )
			return false;

		for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
		{
			ptrdiff_t nBaseDiff = m_vecBaseRegist[i].m_nBaseOff;
			if( nDiff < nBaseDiff )
				continue;
			if( m_vecBaseRegist[i].m_pBaseInfo->IsBaseObject( nDiff - nBaseDiff ) )
				return true;
		}

		// 没有适配的基类
		return false;
	}
}