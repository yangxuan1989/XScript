﻿#ifndef __CALL_BASE_H__
#define __CALL_BASE_H__
//=====================================================================
// CCallBase.h 
// 定义基本的脚本和C++互相调用接口
// 柯达昭
// 2007-10-16
//=====================================================================
#include "CTypeBase.h"
#include <list>
#include "common/CVirtualFun.h"

using namespace std;

namespace Gamma
{
	enum ECallingType
	{
		eCT_TempFunction		= -5,
		eCT_GlobalFunction		= -4,
		eCT_ClassStaticFunction	= -3,
		eCT_ClassFunction		= -2,
		eCT_MemberFunction		= -1,
		eCT_ClassCallBack		= 0,
	};

    class CScriptBase;
    class CTypeBase;
    class CCallBase
    {
	protected:
		CScriptBase*			m_pScript;
		CTypeBase*				m_pThis;
		list<CTypeBase*>		m_listParam;
        CTypeBase*				m_pResult;
		uint32					m_nParamSize;
		uint32					m_nParamCount;
		int32					m_nFunIndex;
		string					m_sFunName;

    public:
        CCallBase( CScriptBase& Script, const STypeInfoArray& aryTypeInfo, 
			int32 nFunIndex, const char* szTypeInfoName, const string& strFunName );

		virtual ~CCallBase(void);
		CScriptBase*			GetScript()					{ return m_pScript; }
		const list<CTypeBase*>& GetParamList()				{ return m_listParam; }
		CTypeBase*				GetResultType()				{ return m_pResult; }
		CTypeBase*				GetThisType()				{ return m_pThis; }
		uint32					GetParamSize()				{ return m_nParamSize; }
		uint32					GetParamCount()				{ return m_nParamCount; }
		int32					GetFunctionIndex()			{ return m_nFunIndex; }
		const string&			GetFunctionName()			{ return m_sFunName; }
		bool					IsCallback()				{ return m_nFunIndex >= eCT_ClassCallBack; }
    };

    //=====================================================================
    // 脚本调用C++的基本接口
    //=====================================================================
    class CByScriptBase 
		: public CCallBase
    {
		const CByScriptBase& operator= ( const CByScriptBase& );
    public:
        CByScriptBase( CScriptBase& Script, const STypeInfoArray& aryTypeInfo, 
			IFunctionWrap* funWrap, const char* szTypeInfoName, int32 nFunIndex, const char* szFunName );

		virtual void	Call( void* pObject, void* pRetBuf, void** pArgArray );
		IFunctionWrap*	GetFunWrap() const { return m_funWrap; }
	protected:
		IFunctionWrap*	m_funWrap;
	};

	//=====================================================================
	// Lua脚本访问C++的成员接口
	//=====================================================================
	class CByScriptMember : public CByScriptBase
	{
		IFunctionWrap*	m_funSet;
	public:
		CByScriptMember( CScriptBase& Script, const STypeInfoArray& aryTypeInfo, 
			IFunctionWrap* funGetSet[2], const char* szTypeInfoName, const char* szFunName );
		virtual void	Call( void* pObject, void* pRetBuf, void** pArgArray );
		IFunctionWrap*	GetFunSet() const { return m_funSet; }
	};

    //=====================================================================
    // C++调用脚本的基本接口
    //=====================================================================
    class CCallScriptBase 
		: public CByScriptBase
		, public ICallBackWrap
	{
		const CCallScriptBase& operator= ( const CCallScriptBase& );
    public:
        CCallScriptBase( CScriptBase& Script, const STypeInfoArray& aryTypeInfo, 
			IFunctionWrap* funWrap, const char* szTypeInfoName, const char* szFunName );
        ~CCallScriptBase();

		void*			GetBootFun()				{ return m_pBootFun; }
		uint32			GetFunIndex()				{ return m_nFunIndex; }

		virtual int32	BindFunction( void* pFun, bool bPureVirtual );
		virtual int32	OnCall( void* pObject, void* pRetBuf, void** pArgArray );

	protected:
		void*			m_pBootFun;
		bool			m_bPureVirtual;

		int32			CallBack( SVirtualObj* pObject, void* pRetBuf, void** pArgArray );
		int32			Destruc( SVirtualObj* pObject, void* pParam );
		virtual void	Call( void* pObject, void* pRetBuf, void** pArgArray );

		virtual bool    CallVM( SVirtualObj* pObject, void* pRetBuf, void** pArgArray ) = 0;
		virtual void	DestrucVM( SVirtualObj* pObject ) = 0;
	};
}

#endif