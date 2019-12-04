﻿//=============================================================
// GammaScriptWrap.h 
// 生成函数调用以及回调的包装
// 柯达昭
// 2012-08-09
//=====================================================================
#pragma once
#include <array>
#include "core/CScriptBase.h"

namespace Gamma
{
	//=======================================================================
	// 获取原始虚表
	//=======================================================================
	typedef SFunctionTable* ( *GetVirtualTableFun )( void* );
	template<typename _T, bool bDuplicatable> struct TCopy 
	{ TCopy( void* pDest, void* pSrc ) { *(_T*)pDest = *(_T*)pSrc; } };
	template<typename _T> struct TCopy<_T, false> 
	{ TCopy( void* pDest, void* pSrc ) { throw( "Can not duplicate object" ); } };

	template<typename ClassType>
	struct TGetVTable : public ClassType
	{
		static GetVirtualTableFun& GetFunInst()
		{
			static GetVirtualTableFun s_fun;
			return s_fun;
		}

		static Gamma::SFunctionTable*& GetVTbInst()
		{
			static Gamma::SFunctionTable* s_table;
			return s_table;
		}

		TGetVTable()
		{
			if( !GetFunInst() || GetVTbInst() )
				return;
			GetVTbInst() = GetFunInst()( this );
		}
	};

	template<>
	struct TGetVTable<void> 
	{
		static GetVirtualTableFun& GetFunInst()
		{
			static GetVirtualTableFun s_fun;
			return s_fun;
		}
	};

	//=======================================================================
	// 构造对象
	//=======================================================================
	template<typename GetVTableType, typename ClassType, bool bDuplicatable>
	struct TConstruct : public IObjectConstruct
	{
		virtual void Assign( void* pDest, void* pSrc )
		{
			TCopy<ClassType, bDuplicatable>( pDest, pSrc );
		}

		virtual void Construct( void* pObj )
		{
			ClassType* pNew = new( pObj )ClassType;
			if( !GetVTableType::GetFunInst() )
				return;
			( ( Gamma::SVirtualObj* )pNew )->m_pTable = GetVTableType::GetVTbInst();
		}

		virtual void Destruct( void* pObj )
		{
			static_cast<ClassType*>( pObj )->~ClassType();
		}

		static IObjectConstruct* Inst()
		{
			static TConstruct<GetVTableType, ClassType, bDuplicatable> s_Instance;
			return &s_Instance;
		}
	};

	template<typename ClassType, bool bDuplicatable>
	struct TConstruct<TGetVTable<void>, ClassType, bDuplicatable>
	{ static IObjectConstruct* Inst() { return nullptr; } };

	//=======================================================================
	// 函数注册链
	//=======================================================================
	class CScriptRegisterNode : public TList<CScriptRegisterNode>::CListNode
	{
		void( *m_funRegister )( );
		typedef typename TList<CScriptRegisterNode>::CListNode ParentType;
	public:
		CScriptRegisterNode( TList<CScriptRegisterNode>& list, void( *fun )() )
			: m_funRegister( fun )
		{
			list.PushBack( *this );
		}

		bool Register()
		{
			m_funRegister();
			auto n = GetNext();
			Remove();
			return n ? n->Register() : true;
		}
	};

	//=======================================================================
	// 获取继承关系信息
	//=======================================================================
	typedef TList<CScriptRegisterNode> CScriptRegisterList;
	struct SGlobalExe { SGlobalExe( bool b = false ) {} };

	template<typename _Derive, typename ... _Base>
	class TInheritInfo
	{
	public:
		enum { size = sizeof...( _Base ) + 1 };
		template<typename...Param> struct TOffset {};
		template<> struct TOffset<> { static void Get( ptrdiff_t* ary ) {} };

		template<typename First, typename...Param>
		struct TOffset<First, Param...>
		{
			static void Get( ptrdiff_t* ary )
			{
				*ary = ( (ptrdiff_t)(First*)(_Derive*)0x40000000 ) - 0x40000000;
				TOffset<Param...>::Get( ++ary );
			}
		};

		static std::array<ptrdiff_t, size + 1> Values()
		{
			std::array<ptrdiff_t, size + 1> result = { sizeof( _Derive ) };
			TOffset<_Base...>::Get( &result[1] );
			return result;
		}

		static std::array<const char*, size + 1> Types()
		{
			return { typeid( _Derive ).name(), typeid( _Base... ).name()... };
		}
	};

	//=======================================================================
	// 获取函数类型
	//=======================================================================
	template<typename RetType, typename... Param>
	inline STypeInfoArray MakeFunArg( RetType ( *pFun )( Param... ) )
	{
		static STypeInfo aryInfo[] =
		{ GetTypeInfo<Param>()..., GetTypeInfo<RetType>() };
		STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
		return TypeInfo;
	}

	//=======================================================================
	// 函数调用包装
	//=======================================================================
	template< typename T >
	struct ArgFetcher
	{
		static inline T&	CallWrapArg( void* pArgArray )	{ return *(T*)pArgArray; }
		static inline void* CallBackArg( T*& pArgArray )	{ return pArgArray; }
	};

	template<typename T>
	struct ArgFetcher<T&>
	{
		static inline T&	CallWrapArg( void* pArgArray )	{ return **(T**)pArgArray; }
		static inline void* CallBackArg( T*& pArgArray )	{ return &pArgArray; }
	};
	
	template<> struct ArgFetcher<const char		&> : public ArgFetcher<char		> {};
	template<> struct ArgFetcher<const int8		&> : public ArgFetcher<int8		> {};
	template<> struct ArgFetcher<const int16	&> : public ArgFetcher<int16	> {};
	template<> struct ArgFetcher<const int32	&> : public ArgFetcher<int32	> {};
	template<> struct ArgFetcher<const int64	&> : public ArgFetcher<int64	> {};
	template<> struct ArgFetcher<const long		&> : public ArgFetcher<long		> {};
	template<> struct ArgFetcher<const wchar_t	&> : public ArgFetcher<wchar_t	> {};
	template<> struct ArgFetcher<const uint8	&> : public ArgFetcher<uint8	> {};
	template<> struct ArgFetcher<const uint16	&> : public ArgFetcher<uint16	> {};
	template<> struct ArgFetcher<const uint32	&> : public ArgFetcher<uint32	> {};
	template<> struct ArgFetcher<const uint64	&> : public ArgFetcher<uint64	> {};
	template<> struct ArgFetcher<const ulong	&> : public ArgFetcher<ulong	> {};
	template<> struct ArgFetcher<const float	&> : public ArgFetcher<float	> {};
	template<> struct ArgFetcher<const double	&> : public ArgFetcher<double	> {};

	template<typename RetType, typename... Param>
	class TFunctionWrap : public IFunctionWrap
	{
		typedef RetType(*FunctionType)(Param...); 

		template< typename Type > struct TCallFunction
		{
			TCallFunction( FunctionType funCall, void* pRetBuf, Param...p )
			{ new( pRetBuf ) Type( funCall( pRetBuf, p...) ); }
		};

		template< typename Type > struct TCallFunction<Type&>
		{
			TCallFunction( FunctionType funCall, void* pRetBuf, Param...p )
			{ *(Type**)pRetBuf = &( funCall( pRetBuf, p... ) ); }
		};

		template<> struct TCallFunction<void>
		{
			TCallFunction( FunctionType funCall, void* pRetBuf, Param...p )
			{ funCall( pRetBuf, p... ); }
		};

		template<typename... RemainParam> struct TFetchParam {};
		template<> struct TFetchParam<>
		{
			template<typename... FetchParam>
			static void CallFun( size_t nIndex, FunctionType funCall, 
				void* pRetBuf, void** pArgArray, FetchParam...p )
			{ TCallFunction<RetType> Temp( funCall, pRetBuf, p... ); }
		};

		template<typename FirstParam, typename... RemainParam>
		struct TFetchParam<FirstParam, RemainParam...>
		{
			template<typename... FetchParam>
			static void CallFun( size_t nIndex, FunctionType funCall, 
				void* pRetBuf, void** pArgArray, FetchParam...p )
			{ TFetchParam<RemainParam...>::CallFun( nIndex + 1, funCall, pRetBuf, 
				pArgArray, p..., ArgFetcher<FirstParam>::CallWrapArg( pArgArray[nIndex] ) ); }
		};

	public:
		void Call( void* orgFun, void* pRetBuf, void** pArgArray )
		{
			TFetchParam<Param...>::CallFun( 0, (FunctionType)orgFun, pRetBuf, pArgArray );
		}

		static TFunctionWrap* GetInst()
		{ 
			static TFunctionWrap s_Inst; 
			return &s_Inst;
		}
	};

	template<typename RetType, typename... Param>
	inline IFunctionWrap* CreateFunWrap( RetType (*orgFun )( Param... ) )
	{ return TFunctionWrap<RetType, Param...>::GetInst(); }

	//=======================================================================
	// 类非常量成员函数回调包装
	//=======================================================================
	template< typename T >
	struct TCallBack
	{
		static T OnCall( uint32 nCallBackIndex, void* pObject, void** pArgArray )
		{
			T ReturnValue;
			CScriptBase::CallBack( nCallBackIndex, pObject, &ReturnValue, pArgArray );
			return ReturnValue;
		}
	};

	template<typename T>
	struct TCallBack<T&>
	{
		static T& OnCall( uint32 nCallBackIndex, void* pObject, void** pArgArray )
		{
			T* pReturnValue;
			int32 ret = CScriptBase::CallBack( nCallBackIndex, pObject, &pReturnValue, pArgArray );
			// 纯虚函数回调，但回调没有实现
			if( ret < 0 )
				throw "i can do nothing here!!!";
			return *pReturnValue;
		}
	};

	template<>
	struct TCallBack<void>
	{
		static void OnCall( uint32 nCallBackIndex, void* pObject, void** pArgArray )
		{
			CScriptBase::CallBack( nCallBackIndex, pObject, NULL, pArgArray );
		}
	};

	template<typename ClassFunType>
	class TCallBackBinder
	{
		template<typename RetType, typename ClassType, typename... Param >
		class TCallBackWrap
		{
		public:
			static int32& GetCallBackIndex()
			{
				static int32 s_nCallBackIndex = -1;
				return s_nCallBackIndex;
			}

			static bool SetCallBack( int32 nIndex, bool bPureVirtual )
			{
				if( nIndex == GetCallBackIndex() )
					return true;
				assert( GetCallBackIndex() < 0 );
				GetCallBackIndex() = nIndex;
				return true;
			}

			template<typename... ParamPtr >
			RetType WrapAddress( ParamPtr ... p )
			{
				void* pCallArray[] = { ArgFetcher<Param...>::CallBackArg( p... ), NULL };
				return TCallBack<RetType>::OnCall( GetCallBackIndex(), this, pCallArray );
			}

			template<>
			RetType WrapAddress<>()
			{
				void* pCallArray[] = { NULL };
				return TCallBack<RetType>::OnCall( GetCallBackIndex(), this, pCallArray );
			}

			RetType Wrap( Param ... p )
			{
				return WrapAddress( &p ... );
			}
		};
	public:
		static Gamma::SFunctionTable* GetVirtualTable( void* p )
		{ 
			return ( (SVirtualObj*)(ClassFunType*)p )->m_pTable; 
		}

		static bool InsallGetVirtualTable()
		{
			ClassFunType::GetFunInst() = (GetVirtualTableFun)&GetVirtualTable;
			return false;
		}

		template<typename RetType, typename ClassType, typename... Param >
		static void Bind( bool bPureVirtual, const char* szFunName,
			RetType(ClassType::*pFun)(Param...) const, void* callOrgFun)
		{
			typedef RetType(*CallOrgFunType)(ClassType*, Param...);
			IFunctionWrap* pWrap = CreateFunWrap(（CallOrgFunType)callOrgFun );
			STypeInfoArray InfoArray = MakeFunArg(（CallOrgFunType)callOrgFun);
			const char* szClassType = typeid( ClassType ).name();
			int32 nIndex = GetVirtualFunIndex( pFun );
			ICallBackWrap& CBWrap = CScriptBase::RegistClassCallback( 
				pWrap, funOrg, InfoArray, szClassType, szFunName );
			typedef TCallBackWrap<RetType, ClassType, Param...> CallBackWrap;
			CBWrap.BindFunction( GetFunAdress(&CallBackWrap::Wrap), bPureVirtual);
			CallBackWrap::SetCallBack( nIndex, bPureVirtual);
		}

		template< typename ClassType, typename RetType, typename... Param >
		static inline void Bind(bool bPureVirtual, const char* szFunName, 
			RetType (ClassType::*pFun)(Param...) const, void* callOrgFun )
		{
			typedef RetType(ClassType::*FunctionType)(Param...);
			Bind( bPureVirtual, szFunName, (FunctionType)pFun, callOrgFun );
		}
	};

	//=======================================================================
	// 析构函数调用包装
	//=======================================================================
	template< typename ClassType >
	class TDestructorWrap : public IFunctionWrap
	{
	public:
		void Call( void* pObj, void* pRetBuf, void** pArgArray, SFunction funRaw )
		{
			class Derive : public ClassType { public: ~Derive() {}; };
			( (Derive*)pObj )->~Derive();
		}
	};

	template< typename ClassType >
	inline IFunctionWrap* CreateDestructorWrap()
	{
		static TDestructorWrap<ClassType> s_instance;
		return &s_instance;
	}

	//=======================================================================
	// 析构函数调用包装绑定
	//=======================================================================
	template<typename ClassType>
	static inline void BindDestructorWrap( ICallBackWrap& CallBackWrap, int32 nIndex )
	{
		static int32 s_nIndex = nIndex;
		class _FunWrap
		{
		public:
			void Wrap( uint32 p0 )	
			{ 
				void* pArg = &p0;
				CScriptBase::CallBack( s_nIndex, this, NULL, &pArg );
			}
		};

		CallBackWrap.BindFunction( GetFunAdress( &_FunWrap::Wrap ), false );
	}

	//=======================================================================
	// 成员读取包装
	//=======================================================================
	template< typename ClassType, typename MemberType >
	class TMemberGetWrap : public IFunctionWrap
	{
	public:
		void Call( void* pObj, void* pRetBuf, void** pArgArray, SFunction funRaw )
		{ new( pRetBuf ) MemberType( *(MemberType*)( (char*)pObj + funRaw.offset ) ); }
		static IFunctionWrap* GetInst() { static TMemberGetWrap s_Inst; return &s_Inst; }
	};

	template< typename ClassType, typename MemberType >
	class TMemberGetWrapObject : public IFunctionWrap
	{
	public:
		void Call( void* pObj, void* pRetBuf, void** pArgArray, SFunction funRaw )
		{ *(MemberType**)pRetBuf = (MemberType*)( (char*)pObj + funRaw.offset ); }
		static IFunctionWrap* GetInst() { static TMemberGetWrapObject s_Inst; return &s_Inst; }
	};

	template< typename ClassType, typename MemberType >
	inline IFunctionWrap* CreateMemberGetWrap( ClassType*, MemberType* )
	{
		STypeInfo TypeInfo;
		GetTypeInfo<MemberType>( TypeInfo );
		if( ( TypeInfo.m_nType >> 24 ) != eDT_custom_type ||
			( ( TypeInfo.m_nType >> 20 )&0xf ) >= eDTE_Pointer ||
			( ( TypeInfo.m_nType >> 16 )&0xf ) >= eDTE_Pointer ||
			( ( TypeInfo.m_nType >> 12 )&0xf ) >= eDTE_Pointer ||
			( ( TypeInfo.m_nType >>  8 )&0xf ) >= eDTE_Pointer ||
			( ( TypeInfo.m_nType >>  4 )&0xf ) >= eDTE_Pointer ||
			( ( TypeInfo.m_nType       )&0xf ) >= eDTE_Pointer )
			return TMemberGetWrap<ClassType, MemberType>::GetInst();
		return TMemberGetWrapObject<ClassType, MemberType>::GetInst();
	}

	//=======================================================================
	// 成员写入包装
	//=======================================================================
	template< typename ClassType, typename MemberType >
	class TMemberSetWrap : public IFunctionWrap
	{
	public:
		void Call( void* pObj, void* pRetBuf, void** pArgArray, SFunction funRaw )
		{ 
			*(MemberType*)( (char*)pObj + funRaw.offset ) = 
			ArgFetcher<MemberType>::CallWrapArg( pArgArray[0] );
		}
		static IFunctionWrap* GetInst() { static TMemberSetWrap s_Inst; return &s_Inst; }
	};

	template< typename ClassType, typename MemberType >
	inline IFunctionWrap* CreateMemberSetWrap( ClassType*, MemberType* )
	{
		return TMemberSetWrap<ClassType, MemberType>::GetInst();
	}

	template< typename ClassType, typename MemberType >
	inline STypeInfoArray MakeMemberArg( ClassType*, MemberType* )
	{
		static STypeInfo aryInfo[2];
		GetTypeInfo<ClassType*>( aryInfo[0] );
		GetTypeInfo<MemberType>( aryInfo[1] );
		if( ( aryInfo[1].m_nType >> 24 ) == eDT_custom_type &&
			( ( aryInfo[1].m_nType >> 20 )&0xf ) < eDTE_Pointer &&
			( ( aryInfo[1].m_nType >> 16 )&0xf ) < eDTE_Pointer &&
			( ( aryInfo[1].m_nType >> 12 )&0xf ) < eDTE_Pointer &&
			( ( aryInfo[1].m_nType >>  8 )&0xf ) < eDTE_Pointer &&
			( ( aryInfo[1].m_nType >>  4 )&0xf ) < eDTE_Pointer &&
			( ( aryInfo[1].m_nType       )&0xf ) < eDTE_Pointer )
			GetTypeInfo<MemberType&>( aryInfo[1] );
		STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
		return TypeInfo;
	}	
}
