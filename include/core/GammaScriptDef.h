﻿#ifndef __GAMMA_SCRIPT_BASE_H__
#define __GAMMA_SCRIPT_BASE_H__
//=====================================================================
// GammaScriptBase.h
// 定义脚本和C++接口的基本数据类型
// 柯达昭
// 2007-10-16
//=====================================================================
#include "common/CommonType.h"
#include "common/GammaCppParser.h"
#include <string>

#if ( defined( _WIN32 ) && defined( GAMMA_DLL ) )
	#if defined( GAMMA_SCRIPT_EXPORTS )
		#define GAMMA_SCRIPT_API __declspec(dllexport)
	#else
		#define GAMMA_SCRIPT_API __declspec(dllimport)
	#endif
#else
	#define GAMMA_SCRIPT_API 
#endif

namespace Gamma
{
#define MAX_BASE_CLASS_NUM 5
#define MAX_FUNCTION_PARAM_NUM 10

	class CScriptBase;

	struct STypeInfoArray
	{
		STypeInfo*	aryInfo;
		uint32		nSize;
	};

	class IObjectConstruct
	{
	public:
		virtual void		Assign( void*, void* ) = 0;
		virtual void		Construct( void* ) = 0;
		virtual void		Destruct( void* ) = 0;
	};

	class IFunctionWrap
	{
	public:
		virtual void		Call( void* pRetBuf, void** pArgArray, uintptr_t funRaw ) = 0;
	};
}


#endif
