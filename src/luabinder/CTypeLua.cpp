﻿#include "zlib.h"
#include <string>

extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lstate.h"
	#include "lualib.h"
}

#include "common/Help.h"
#include "common/CodeCvs.h"
#include "CTypeLua.h"
#include "CScriptLua.h"
#include "core/CClassRegistInfo.h"

using namespace std;

namespace Gamma
{
    //=====================================================================
    // lua数据类型之C++对象类型
	//=====================================================================
	static void ConstructLua( lua_State* pL )
	{
		lua_getfield( pL, -1, "Ctor" );
		if( !lua_isnil( pL, -1 ) )
		{
			lua_pushvalue( pL, -2 );
			lua_call( pL, 1, 0 );
		}
		else
		{
			lua_pop( pL, 1 );
		}
	}

    CLuaObject::CLuaObject( CClassRegistInfo* pClassInfo, uint32 nSize )
        : CLuaPointer( nSize )
		, m_pClassInfo( pClassInfo )
    { 
		m_nType = eDT_class;
		m_nFlag = 0;
    }

    void CLuaObject::GetFromVM( lua_State* pL, char* pDataBuf, int32 nStkId, bool bExtend32Bit )
    {
        nStkId = AbsStackIdx( pL, nStkId );
		int32 nType = lua_type( pL, nStkId );
        if( nType == LUA_TNIL || nType == LUA_TNONE )
            *(void**)( pDataBuf ) = NULL;    
        else
        {
			if( !lua_istable( pL, nStkId )  )
			{
				luaL_error( pL, "GetFromVM error id:%d", nStkId );
				return;
			}
			
			lua_getfield( pL, nStkId, m_pClassInfo->GetObjectIndex().c_str() );
			if( lua_isnil( pL, -1 ) )
			{
				lua_pushstring( pL, s_szLuaBufferInfo );
				lua_rawget( pL, nStkId );
				if( lua_islightuserdata( pL, -1 ) || lua_type( pL, -1 ) == LUA_TUSERDATA )
				{
					SBufferInfo* pInfo = (SBufferInfo*)lua_touserdata( pL, -1 );
					*(void**)( pDataBuf ) = pInfo ? pInfo->pBuffer : NULL;
					lua_pop( pL, 2 );
				}
				else
				{
					*(void**)( pDataBuf ) = NULL;
					lua_pop( pL, 2 );
				}
				return;
			}

            *(void**)( pDataBuf ) = lua_touserdata( pL, -1 );
            lua_pop( pL, 1 );
        }
    }

    void CLuaObject::PushToVM( lua_State* pL, char* pDataBuf )
    {
        void* pObj = *(void**)( pDataBuf );
        if( pObj == NULL )
        {
            lua_pushnil( pL );
            return;
        }

		lua_pushlightuserdata( pL, CScriptLua::ms_pGlobObjectTableKey );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		if( lua_isnil( pL, -1 ) )
		{
			luaL_error( pL, "PushToVM error param" );
			return;
		}

        lua_pushlightuserdata( pL, pObj );
        lua_gettable( pL, -2 );

        if( !lua_isnil( pL, -1 ) )
        {
            const gammacstring& sObjectIndex = m_pClassInfo->GetObjectIndex();
            lua_getfield( pL, -1, sObjectIndex.c_str() );
            bool bNil = lua_isnil( pL, -1 );
            lua_pop( pL, 1 );
            if( !bNil )
            {
                lua_remove( pL, -2 );
                return;
            }

            CScriptLua::GetScript( pL )->UnlinkCppObjFromScript( pObj );
        }

        lua_pop( pL, 2 );

		// Table 在Lua栈顶
		lua_newtable( pL );
		lua_getglobal( pL, m_pClassInfo->GetClassName().c_str() );
		if( lua_isnil( pL, -1 ) )
		{
			luaL_error( pL, "PushToVM Class Not Registed:%s", m_pClassInfo->GetClassName().c_str() );//szClass必须被注册
			return;
			
		}
		lua_setmetatable( pL, -2 );

		// 绑定ObjectIndex
		lua_pushstring( pL, m_pClassInfo->GetObjectIndex().c_str() );
		lua_pushlightuserdata( pL, *(void**)( pDataBuf ) );
		lua_rawset( pL, -3 );

		CScriptLua::RegisterObject( pL, m_pClassInfo, *(void**)( pDataBuf ), false );
		ConstructLua( pL );
    }

    //=====================================================================
    // lua数据类型之C++对象类型
    //=====================================================================
    CLuaValueObject::CLuaValueObject( CClassRegistInfo* pClassInfo )
        : CLuaObject( pClassInfo, pClassInfo->GetClassSize() )
	{
		m_nFlag = eFPT_Value; 
	}

	void CLuaValueObject::GetFromVM( lua_State* pL, char* pDataBuf, int32 nStkId, bool bExtend32Bit )
	{
		void* pObject = NULL;
		CLuaObject::GetFromVM( pL, (char*)&pObject, nStkId, bExtend32Bit );
		m_pClassInfo->Assign( pDataBuf, pObject );
	}

	void CLuaValueObject::PushToVM( lua_State* pL, char* pDataBuf )
	{
		if( !IsReturn() )
		{
			void* pObject = pDataBuf;
			return CLuaObject::PushToVM( pL, (char*)&pObject );
		}

		// Table 在Lua栈顶
		lua_newtable( pL );// Obj
		int32 nStkId = AbsStackIdx( pL, -1 );
		
		lua_getglobal( pL, m_pClassInfo->GetClassName().c_str() );
		lua_setmetatable( pL, nStkId );

		CScriptLua::NewLuaObj( pL, m_pClassInfo, pDataBuf );
		ConstructLua( pL );
		m_pClassInfo->Release( pDataBuf );
	}

	//=====================================================================
    // lua数据类型之C++指针类型
    //=====================================================================
    CLuaBuffer::CLuaBuffer()
    {
    }

    void CLuaBuffer::PushToVM( lua_State* pL, char* pDataBuf )
	{
		void* pBuffer = *(void**)( pDataBuf );
		if( pBuffer == NULL )
		{
			lua_pushnil( pL );
			return;
		}

		lua_pushlightuserdata( pL, CScriptLua::ms_pGlobObjectTableKey );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		if( lua_isnil( pL, -1 ) )
		{
			luaL_error( pL, "PushToVM error" );
			return;
		}

		lua_pushlightuserdata( pL, pBuffer );
		lua_gettable( pL, -2 );

		if( !lua_isnil( pL, -1 ) )
		{
			SBufferInfo* pInfo = GetBufferInfo( pL, -1 );
			if( pInfo && pInfo->pBuffer == pBuffer )
			{
				lua_remove( pL, -2 );
				pInfo->nDataSize = (uint32)INVALID_32BITID;
				pInfo->nCapacity = (uint32)INVALID_32BITID;
				pInfo->nPosition = 0;
				return;
			}

			CScriptLua::GetScript( pL )->UnlinkCppObjFromScript( pBuffer );
		}

		lua_pop( pL, 2 );

		lua_newtable( pL );// Obj
		int32 nStkId = AbsStackIdx( pL, -1 );

		lua_getglobal( pL, s_szLuaBufferClass );
		if( lua_isnil( pL, -1 ) )//szClass必须被注册
		{
			luaL_error( pL, "PushToVM Class:%s", s_szLuaBufferClass );
			return;
		}

		lua_setmetatable( pL, nStkId );

		// 设置数据到table上
		lua_pushstring( pL, s_szLuaBufferInfo );
		SBufferInfo* pInfo = (SBufferInfo*)lua_newuserdata( pL, sizeof(SBufferInfo) );
		pInfo->nDataSize = (uint32)INVALID_32BITID;
		pInfo->nCapacity = (uint32)INVALID_32BITID;
		pInfo->nPosition = 0;
		pInfo->pBuffer = (tbyte*)pBuffer;
		lua_rawset( pL, nStkId );

		// 挂到全局表上
		lua_pushlightuserdata( pL, CScriptLua::ms_pGlobObjectTableKey );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		lua_pushlightuserdata( pL, pBuffer );
		lua_pushvalue( pL, nStkId );
		lua_settable( pL, -3 );
		lua_pop( pL, 1 );
    }

    void CLuaBuffer::GetFromVM( lua_State* pL, char* pDataBuf, int32 nStkId, bool bExtend32Bit )
    {
		nStkId = AbsStackIdx( pL, nStkId );
		int32 nType = lua_type( pL, nStkId );
		if( nType == LUA_TNIL || nType == LUA_TNONE )
			*(void**)( pDataBuf ) = NULL;    
		else if( nType == LUA_TTABLE )
		{
			lua_pushstring( pL, s_szLuaBufferInfo );
			lua_rawget( pL, nStkId );
			if( lua_islightuserdata( pL, -1 ) || lua_type( pL, -1 ) == LUA_TUSERDATA )
			{
				SBufferInfo* pInfo = (SBufferInfo*)lua_touserdata( pL, -1 );
				*(void**)( pDataBuf ) = pInfo ? pInfo->pBuffer : NULL;
				lua_pop( pL, 1 );
			}
			else
			{
				*(void**)( pDataBuf ) = NULL;
				lua_pop( pL, 1 );
			}
		}   
		else if( nType == LUA_TSTRING )
		{
			*(const char**)( pDataBuf ) = GetStrFromLua( pL, nStkId );
		}  
		else
		{
			*(void**)( pDataBuf ) = NULL;
		}
	}

	inline bool CLuaBuffer::IsLightData( SBufferInfo* pInfo )
	{
		return (void*)( pInfo + 1 ) != pInfo->pBuffer;
	}

	inline SBufferInfo* CLuaBuffer::CheckBufferSpace( 
		SBufferInfo* pInfo, uint32 nTotalSize, lua_State* pL, int32 nStkID )
	{
		if( pInfo && IsLightData( pInfo ) && nTotalSize > pInfo->nDataSize )
		{
			luaL_error( pL, "can not write data to native buffer over nDataSize" );
			return NULL;
		}

		if( pInfo == NULL || pInfo->pBuffer == NULL || nTotalSize > pInfo->nCapacity )
		{
			lua_pushstring( pL, s_szLuaBufferInfo );
			uint32 nLen = Max<uint32>( nTotalSize, 16 );
			uint32 nCapacity = nLen + nLen/2;
			uint32 nAlocSize = nCapacity + sizeof(SBufferInfo);
			SBufferInfo* pNewBufferInfo = (SBufferInfo*)lua_newuserdata( pL, nAlocSize );
			pNewBufferInfo->pBuffer = (tbyte*)( pNewBufferInfo + 1 );
			pNewBufferInfo->nPosition = pInfo ? pInfo->nPosition : 0;
			pNewBufferInfo->nDataSize = pInfo ? pInfo->nDataSize : 0;
			pNewBufferInfo->nCapacity = nCapacity;

			if( pInfo && pInfo->pBuffer )
				memcpy( pNewBufferInfo->pBuffer, pInfo->pBuffer, pInfo->nDataSize );
			pInfo = pNewBufferInfo;
			lua_rawset( pL, nStkID );
			uint32 nNewStart = pInfo->nDataSize;
			uint32 nNewSize = nCapacity - pInfo->nDataSize;
			memset( pNewBufferInfo->pBuffer + nNewStart, 0, nNewSize );
		}	
		return pInfo;
	}

	inline SBufferInfo* CLuaBuffer::GetBufferInfo( lua_State* pL, int32 nStkID )
	{
		nStkID = AbsStackIdx( pL, nStkID );
		lua_pushstring( pL, s_szLuaBufferInfo );
		lua_rawget( pL, nStkID );
		SBufferInfo* pInfo = (SBufferInfo*)lua_touserdata( pL, -1 );
		lua_pop( pL, 1 );
		return pInfo;
	}

	template<class Type>
	Type CLuaBuffer::ReadData( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || pInfo->nPosition + sizeof(Type) > pInfo->nDataSize )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}
		Type data;
		memcpy( &data, pInfo->pBuffer + pInfo->nPosition, sizeof(Type) );
		pInfo->nPosition += sizeof(Type);
		return data;
	}

	int32 CLuaBuffer::GetBit( lua_State* pL )
	{
		uint32 nArg = lua_gettop( pL );
		if( nArg < 2 )
		{
			luaL_error( pL, "GetBit Invalid Param" );
			return 0;
		}

		uint32 nReadPos = GetNumFromLua( pL, 2 );
		uint32 nNum = 1;
		if( nArg > 3 )
			nNum = GetNumFromLua( pL, 3 );
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( nNum <= 0 || nNum > 64 )
			return 0;
		if( nReadPos + nNum > pInfo->nDataSize * 8 )
			return 0;
		int32 nOffset = nReadPos%8;
		int32 nStart = nReadPos/8;
		int32 nEnd = ( nReadPos + nNum - 1 )/8 + 1;
		char aryBuffer[9];
		memcpy( aryBuffer, pInfo->pBuffer, nEnd - nStart );
		uint64& nValue = *(uint64*)aryBuffer;
		nValue = nValue >> nOffset;
		aryBuffer[7] |= aryBuffer[8] << ( 8 - nOffset );
		lua_pushnumber( pL, (double)(int64)nValue );
		return 1;
	}

	int32 CLuaBuffer::ReadBoolean( lua_State* pL )
	{
		lua_pushboolean( pL, ReadData<uint8>( pL ) );		
		return 1;
	}

	int32 CLuaBuffer::ReadInt8( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<int8>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadDouble( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<double>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadFloat( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<float>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadInt64( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<int64>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadInt32( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<int32>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadInt16( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<int16>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadUint8( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<uint8>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadUint64( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<uint64>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadUint32( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<uint32>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadUint16( lua_State* pL )
	{
		lua_pushnumber( pL, ReadData<uint16>( pL ) );
		return 1;
	}

	int32 CLuaBuffer::ReadUTF( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || pInfo->nPosition + sizeof(uint16) > pInfo->nDataSize )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}
		uint16 nLen;
		memcpy( &nLen, pInfo->pBuffer + pInfo->nPosition, sizeof(uint16) );
		const char* szUtf8 = (const char*)( pInfo->pBuffer + pInfo->nPosition + sizeof(uint16) );
		
		if( pInfo->nPosition + sizeof(uint16) + nLen > pInfo->nDataSize )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}
		lua_pushlstring( pL, szUtf8, nLen );
		pInfo->nPosition += sizeof(uint16) + nLen;
		return 1;
	}

	int32 CLuaBuffer::ReadUTFBytes( lua_State* pL )
	{
		uint32 nLen = GetNumFromLua( pL, -1 );
		lua_pop( pL, 1 );

		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || pInfo->nPosition + nLen > pInfo->nDataSize )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}
		const char* szUtf8 = (const char*)pInfo->pBuffer + pInfo->nPosition;
		lua_pushlstring( pL, szUtf8, strnlen( szUtf8, nLen ) );
		pInfo->nPosition += nLen;
		return 1;
	}

	int32 CLuaBuffer::ReadUCS( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || pInfo->nPosition + sizeof(uint16) > pInfo->nDataSize )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}


		uint16 nLen;
		memcpy( &nLen, pInfo->pBuffer + pInfo->nPosition, sizeof(uint16) );
		const uint16* szUtf16 = (const uint16*)( pInfo->pBuffer + pInfo->nPosition + sizeof(uint16) );
		if( pInfo->nPosition + sizeof(uint16) + nLen*sizeof(uint16) > pInfo->nDataSize)
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}
		pInfo->nPosition += sizeof(uint16) + nLen*sizeof(uint16);
		CScriptLua* pScript = CScriptLua::GetScript( pL );
		pScript->m_szTempUcs2.resize( nLen );
		for( uint32 i = 0; i < nLen; i++ )
			pScript->m_szTempUcs2[i] = szUtf16[i];
		pScript->m_szTempUtf8.resize( nLen*3 + 1 );
		nLen = UcsToUtf8( &pScript->m_szTempUtf8[0], nLen*3 + 1, pScript->m_szTempUcs2.c_str() );
		lua_pushlstring( pL, pScript->m_szTempUtf8.c_str(), nLen );
		return 1;
	}

	int32 CLuaBuffer::ReadUCSCounts( lua_State* pL )
	{
		uint32 nLen = GetNumFromLua( pL, 2 );
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || pInfo->nPosition + nLen*sizeof(uint16) > pInfo->nDataSize )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}

		lua_settop( pL, 0 );

		const uint16* szUtf16 = (const uint16*)( pInfo->pBuffer + pInfo->nPosition );
		pInfo->nPosition += nLen*sizeof(uint16);

		CScriptLua* pScript = CScriptLua::GetScript( pL );
		pScript->m_szTempUcs2.resize( nLen );
		for( uint32 i = 0; i < nLen; i++ )
			pScript->m_szTempUcs2[i] = szUtf16[i];
		pScript->m_szTempUtf8.resize( nLen*3 + 1 );
		nLen = UcsToUtf8( &pScript->m_szTempUtf8[0], nLen*3 + 1, pScript->m_szTempUcs2.c_str() );
		lua_pushlstring( pL, pScript->m_szTempUtf8.c_str(), nLen );
		return 1;
	}

	int32 CLuaBuffer::ReadBytes( lua_State* pL )
	{
		uint32 nArg = lua_gettop( pL );
		SBufferInfo* pInfoSrc = GetBufferInfo( pL, 1 );

		if( !pInfoSrc || !pInfoSrc->pBuffer )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}

		if( lua_type( pL, 2 ) != LUA_TTABLE )
		{
			uint32 nReadCount = nArg >= 2 ? GetNumFromLua( pL, 2 ) : INVALID_32BITID;

			if( nReadCount == INVALID_32BITID )
				nReadCount = pInfoSrc->nDataSize - pInfoSrc->nPosition;
			lua_settop( pL, 0 );

			if( pInfoSrc->nPosition + nReadCount > pInfoSrc->nDataSize )
			{
				luaL_error( pL, "invalid buffer" );
				return 0;
			}

			lua_pushlstring( pL, (char*)pInfoSrc->pBuffer + pInfoSrc->nPosition, nReadCount );
			pInfoSrc->nPosition += nReadCount;
			return 1;
		}
		else
		{
			SBufferInfo* pInfoDes = GetBufferInfo( pL, 2 );
			uint32 nOffset = nArg >= 3 ? GetNumFromLua( pL, 3 ) : 0;
			uint32 nReadCount = nArg >= 4 ? GetNumFromLua( pL, 4 ) : INVALID_32BITID;

			if( nReadCount == INVALID_32BITID )
				nReadCount = pInfoSrc->nDataSize - pInfoSrc->nPosition;

			if( nReadCount > 200*1024*1024 )
			{
				luaL_error( pL, "invalid size" );
				return 0;
			}

			pInfoSrc = pInfoSrc == pInfoDes ? NULL : pInfoSrc;
			pInfoDes = CheckBufferSpace( pInfoDes, nOffset + nReadCount, pL, 2 );
			lua_settop( pL, 0 );

			pInfoSrc = pInfoSrc == NULL ? pInfoDes : pInfoSrc;
			memmove( pInfoDes->pBuffer + nOffset, pInfoSrc->pBuffer + pInfoSrc->nPosition, nReadCount );
			if( pInfoSrc->nPosition + nReadCount > pInfoSrc->nDataSize )
			{
				luaL_error( pL, "invalid buffer" );
				return 0;
			}
			pInfoSrc->nPosition += nReadCount;
			pInfoDes->nDataSize = Max<uint32>( nOffset + nReadCount, pInfoDes->nDataSize );
			return 0;
		}
	}

	template<class Type>
	void CLuaBuffer::WriteData( lua_State* pL, Type v )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		pInfo = CheckBufferSpace( pInfo, ( pInfo ? pInfo->nPosition : 0 )  + sizeof(Type), pL, 1 );
		memcpy( pInfo->pBuffer + pInfo->nPosition, &v, sizeof(Type) );
		pInfo->nPosition += sizeof(Type);
		pInfo->nDataSize = Max<uint32>( pInfo->nPosition, pInfo->nDataSize );
		lua_settop( pL, 0 );
	}

	int32 CLuaBuffer::SetBit( lua_State* pL )
	{
		uint32 nArg = lua_gettop( pL );
		if( nArg < 3 )
		{
			luaL_error( pL, "invalid parameter count" );
			return 0;
		}

		char aryBuffer[9];
		uint64& nValue = *(uint64*)aryBuffer;
		uint32 nWritePos = GetNumFromLua( pL, 2 );
		nValue = (uint64)( LUA_TBOOLEAN == lua_type( pL, 3 ) ?
			lua_toboolean( pL, 3 ) : (int64)GetNumFromLua( pL, 3 ) );

		uint32 nNum = 1;
		if( nArg > 4 )
			nNum = GetNumFromLua( pL, 4 );
		if( nNum <= 0 || nNum > 64 )
			return 0;
		int32 nOffset = nWritePos%8;
		int32 nStart = nWritePos/8;
		int32 nEnd = ( nWritePos + nNum - 1 )/8 + 1;
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		pInfo = CheckBufferSpace( pInfo, nEnd, pL, 1 );

		aryBuffer[8] = aryBuffer[7] >> ( 8 - nOffset );
		nValue = nValue << nOffset;
		tbyte* pStart = pInfo->pBuffer + nStart;
		pStart[0] = ( pStart[0]&( 0xff >> nOffset ) )|aryBuffer[0];
		memcpy( pStart + 1, aryBuffer + 1, 7 );
		pStart[8] = ( pStart[8]&( 0xff << nOffset ) )|aryBuffer[8];
		return 0;
	}

	int32 CLuaBuffer::WriteBoolean( lua_State* pL )
	{
		WriteData( pL, (uint8)(int64)lua_toboolean( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteInt8( lua_State* pL )
	{
		WriteData( pL, (int8)(int64)GetNumFromLua( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteDouble( lua_State* pL )
	{
		WriteData( pL, GetNumFromLua( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteFloat( lua_State* pL )
	{
		WriteData( pL, (float)GetNumFromLua( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteInt64( lua_State* pL )
	{
		WriteData( pL, (int64)GetNumFromLua( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteInt32( lua_State* pL )
	{
		WriteData( pL, (int32)(int64)GetNumFromLua( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteUint64( lua_State* pL )
	{
		WriteData( pL, (uint64)GetNumFromLua( pL, 2 ) );
		return 0;
	}
	
	int32 CLuaBuffer::WriteInt16( lua_State* pL )
	{
		WriteData( pL, (int16)(int64)GetNumFromLua( pL, 2 ) );
		return 0;
	}

	int32 CLuaBuffer::WriteUTF( lua_State* pL )
	{
		const char* szString = lua_tostring( pL, 2 );
		szString = szString ? szString : "";
		uint16 nSize = (uint16)strlen( szString );
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		uint32 nNewSize = ( pInfo ? pInfo->nPosition : 0 ) + nSize + sizeof(uint16);
		pInfo = CheckBufferSpace( pInfo, nNewSize, pL, 1 );
		lua_settop( pL, 0 );

		memcpy( pInfo->pBuffer + pInfo->nPosition, &nSize, sizeof(uint16) );
		memcpy( pInfo->pBuffer + pInfo->nPosition + sizeof(uint16), szString, nSize );
		pInfo->nPosition += nSize + sizeof(uint16);
		pInfo->nDataSize = Max<uint32>( pInfo->nPosition, pInfo->nDataSize );
		return 0;
	}

	int32 CLuaBuffer::WriteUTFBytes( lua_State* pL )
	{
		uint32 nArg = lua_gettop( pL );
		uint32 nLen = (uint32)lua_strlen( pL, 2 );
		const char* szString = lua_tostring( pL, 2 );
		uint32 nSize = nArg >= 3 ? (uint32)(int64)GetNumFromLua( pL, 3 ) : nLen;
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		pInfo = CheckBufferSpace( pInfo, ( pInfo ? pInfo->nPosition : 0 ) + nSize, pL, 1 );
		lua_settop( pL, 0 );

		szString = szString ? szString : "";
		memcpy( pInfo->pBuffer + pInfo->nPosition, szString, Min( nSize, nLen ) );
		if( nSize > nLen )
			memset( pInfo->pBuffer + pInfo->nPosition + nLen, 0, nSize - nLen );
		pInfo->nPosition += nSize;
		pInfo->nDataSize = Max<uint32>( pInfo->nPosition, pInfo->nDataSize );
		return 0;
	}

	int32 CLuaBuffer::WriteBytes( lua_State* pL )
	{
		uint32 nArg = lua_gettop( pL );
		SBufferInfo* pInfoDes = GetBufferInfo( pL, 1 );

		if( lua_type( pL, 2 ) != LUA_TTABLE )
		{
			size_t nDataSize = 0;
			const char* szSrc = luaL_checklstring( pL, 2, &nDataSize );
			uint32 nOffset = nArg >= 3 ? GetNumFromLua( pL, 3 ) : 0;
			uint32 nWriteCount = nArg >= 4 ? GetNumFromLua( pL, 4 ) : INVALID_32BITID;

			if( nWriteCount == INVALID_32BITID )
				nWriteCount = (uint32)nDataSize - nOffset;

			if( nWriteCount > 200*1024*1024 )
			{
				luaL_error( pL, "invalid size" );
				return 0;
			}

			if( nOffset + nWriteCount > nDataSize )
			{
				luaL_error( pL, "invalid buffer" );
				return 0;
			}

			pInfoDes = CheckBufferSpace( pInfoDes, ( pInfoDes ? pInfoDes->nPosition : 0 ) + nWriteCount, pL, 1 );
			lua_settop( pL, 0 );

			memmove( pInfoDes->pBuffer + pInfoDes->nPosition, szSrc + nOffset, nWriteCount );
			pInfoDes->nPosition += nWriteCount;
			pInfoDes->nDataSize = Max<uint32>( pInfoDes->nPosition, pInfoDes->nDataSize );
			return 0;
		}
		else
		{
			SBufferInfo* pInfoSrc = GetBufferInfo( pL, 2 );
			uint32 nOffset = nArg >= 3 ? (uint32)(int64)GetNumFromLua( pL, 3 ) : 0;
			uint32 nWriteCount = nArg >= 4 ? (uint32)(int64)GetNumFromLua( pL, 4 ) : INVALID_32BITID;

			if( nWriteCount == INVALID_32BITID )
				nWriteCount = pInfoSrc->nDataSize - nOffset;

			if( !pInfoSrc || !pInfoSrc->pBuffer )
			{
				luaL_error( pL, "invalid buffer" );
				return 0;
			}

			if( nWriteCount > 200*1024*1024 )
			{
				luaL_error( pL, "invalid size" );
				return 0;
			}

			pInfoSrc = pInfoSrc == pInfoDes ? NULL : pInfoSrc;
			pInfoDes = CheckBufferSpace( pInfoDes, ( pInfoDes ? pInfoDes->nPosition : 0 ) + nWriteCount, pL, 1 );
			lua_settop( pL, 0 );

			pInfoSrc = pInfoSrc == NULL ? pInfoDes : pInfoSrc;
			memmove( pInfoDes->pBuffer + pInfoDes->nPosition, pInfoSrc->pBuffer + nOffset, nWriteCount );
			if( nOffset + nWriteCount > pInfoSrc->nDataSize )
			{
				luaL_error( pL, "invalid buffer" );
				return 0;
			}
			pInfoDes->nPosition += nWriteCount;
			pInfoDes->nDataSize = Max<uint32>( pInfoDes->nPosition, pInfoDes->nDataSize );
			return 0;
		}
	}

	int32 CLuaBuffer::Uncompress( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || ( IsLightData( pInfo ) && pInfo->pBuffer ) )
		{
			luaL_error( pL,  "invalid buffer" );
			return 0;
		}

		string strBuffer;
		Bytef szBuffer[4096];

		z_stream zipStream;
		memset( &zipStream, 0, sizeof(zipStream) );
		inflateInit( &zipStream );
		zipStream.next_in = pInfo->pBuffer;
		zipStream.avail_in = pInfo->nDataSize;

		while( zipStream.avail_in )
		{
			zipStream.next_out = szBuffer;
			zipStream.avail_out = 4096;
			zipStream.total_out = 0;
			inflate( &zipStream, Z_SYNC_FLUSH );
			strBuffer.append( (char*)szBuffer, zipStream.total_out );
		}

		zipStream.next_out = szBuffer;
		zipStream.avail_out = 4096;
		zipStream.total_out = 0;
		inflate( &zipStream, Z_FINISH );
		inflateEnd( &zipStream );	
		strBuffer.append( (char*)szBuffer, zipStream.total_out );	

		pInfo = CheckBufferSpace( pInfo, (uint32)strBuffer.size(), pL, 1 );
		memcpy( pInfo->pBuffer, strBuffer.c_str(), strBuffer.size() );
		pInfo->nPosition = 0;
		pInfo->nDataSize = (uint32)strBuffer.size();
		lua_settop( pL, 0 );
		return 0;
	}

	int32 CLuaBuffer::Compress( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || ( IsLightData( pInfo ) && pInfo->pBuffer ) )
		{
			luaL_error( pL, "invalid buffer" );
			return 0;
		}

		string strBuffer;
		Bytef szBuffer[4096];

		z_stream zipStream;
		memset( &zipStream, 0, sizeof(zipStream) );
		deflateInit( &zipStream, 9 );
		zipStream.next_in = pInfo->pBuffer;
		zipStream.avail_in = pInfo->nDataSize;

		while( zipStream.avail_in )
		{
			zipStream.next_out = szBuffer;
			zipStream.avail_out = 4096;
			zipStream.total_out = 0;
			deflate( &zipStream, Z_SYNC_FLUSH );
			strBuffer.append( (char*)szBuffer, zipStream.total_out );
		}

		zipStream.next_out = szBuffer;
		zipStream.avail_out = 4096;
		zipStream.total_out = 0;
		deflate( &zipStream, Z_FINISH );
		deflateEnd( &zipStream );	
		strBuffer.append( (char*)szBuffer, zipStream.total_out );	

		pInfo = CheckBufferSpace( pInfo, (uint32)strBuffer.size(), pL, 1 );
		memcpy( pInfo->pBuffer, strBuffer.c_str(), strBuffer.size() );
		pInfo->nPosition = 0;
		pInfo->nDataSize = (uint32)strBuffer.size();
		lua_settop( pL, 0 );
		return 0;
	}

	int32 CLuaBuffer::SetPosition( lua_State* pL )
	{
		uint32 nPosition = (uint32)(int64)GetNumFromLua( pL, 2 );
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( pInfo && IsLightData( pInfo ) )
		{
			if( nPosition >= pInfo->nDataSize )
				nPosition = pInfo->nDataSize;
			pInfo->nPosition = nPosition;
			return 0;
		}
		if( !pInfo )
			pInfo = CheckBufferSpace( pInfo, nPosition, pL, 1 );
		pInfo->nPosition = nPosition;
		lua_settop( pL, 0 );
		return 0;
	}

	int32 CLuaBuffer::GetPosition( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		lua_settop( pL, 0 );
		lua_pushnumber( pL, pInfo ? pInfo->nPosition : 0 );
		return 1;
	}

	int32 CLuaBuffer::SetDataSize( lua_State* pL )
	{
		uint32 nLen = (uint32)(int64)GetNumFromLua( pL, 2 );
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( pInfo && pInfo->pBuffer && IsLightData( pInfo ) )
		{
			if( nLen < pInfo->nCapacity )
				pInfo->nDataSize = nLen;
			return 0;
		}

		pInfo = CheckBufferSpace( pInfo, nLen, pL, 1 );
		pInfo->nDataSize = nLen;
		lua_settop( pL, 0 );
		return 0;
	}

	int32 CLuaBuffer::GetDataSize( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		lua_settop( pL, 0 );
		lua_pushnumber( pL, pInfo ? pInfo->nDataSize : 0 );
		return 1;
	}

	int32 CLuaBuffer::Reset( lua_State* pL )
	{
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );
		if( !pInfo || IsLightData( pInfo ) )
			return 0;
		pInfo->nPosition = 0;
		pInfo->nDataSize = 0;
		memset( pInfo->pBuffer, 0, pInfo->nCapacity );
		return 0;
	}
	
	int32 CLuaBuffer::Clear( lua_State* pL )
	{
		uint32 nArg = lua_gettop( pL );
		uint32 nClearCount = nArg >= 3 ? (uint32)(int64)GetNumFromLua( pL, 3 ) : 1;
		uint32 nOffset = nArg >= 2 ? (uint32)(int64)GetNumFromLua( pL, 2 ) : 0;
		SBufferInfo* pInfo = GetBufferInfo( pL, 1 );

		if( nClearCount == INVALID_32BITID || nArg < 2 )
			nClearCount = pInfo->nDataSize - nOffset;

		pInfo = CheckBufferSpace( pInfo, nOffset + nClearCount, pL, 1 );
		lua_settop( pL, 0 );

		memset( pInfo->pBuffer + nOffset, 0, nClearCount );
		pInfo->nDataSize = Max<uint32>( nOffset + nClearCount, pInfo->nDataSize );
		return 0;
	}

    void CLuaBuffer::RegistClass( CScriptLua* pScript )
	{
		char szSrc[256] = { 0 };
		strcat2array_safe( szSrc, s_szLuaBufferClass );
		strcat2array_safe( szSrc, " = class();" );
		pScript->RunString( szSrc );

		lua_State* pL = pScript->GetLuaState();
		lua_getglobal( pL, s_szLuaBufferClass );

		#define REGISTER( f ) \
		lua_pushcfunction( pL, f ); lua_setfield( pL, -2, #f )

		REGISTER( GetBit );
		REGISTER( ReadBoolean );
		REGISTER( ReadInt8 );
		REGISTER( ReadDouble );
		REGISTER( ReadFloat );
		REGISTER( ReadInt64 );
		REGISTER( ReadInt32 );
		REGISTER( ReadInt16 );
		REGISTER( ReadUint8 );
		REGISTER( ReadUint64 );
		REGISTER( ReadUint32 );
		REGISTER( ReadUint16 );
		REGISTER( ReadUTF );
		REGISTER( ReadUTFBytes );
		REGISTER( ReadUCS );
		REGISTER( ReadUCSCounts );
		REGISTER( ReadBytes );

		REGISTER( SetBit );
		REGISTER( WriteBoolean );
		REGISTER( WriteInt8 );
		REGISTER( WriteDouble );
		REGISTER( WriteFloat );
		REGISTER( WriteInt64 );
		REGISTER( WriteInt32 );
		REGISTER( WriteUint64 );
		REGISTER( WriteInt16 );
		REGISTER( WriteUTF );
		REGISTER( WriteUTFBytes );
		REGISTER( WriteBytes );

		REGISTER( Uncompress );
		REGISTER( Compress );

		REGISTER( SetPosition );
		REGISTER( GetPosition );
		REGISTER( GetDataSize );
		REGISTER( SetDataSize );
		REGISTER( Reset );
		REGISTER( Clear );

        lua_pop( pL, 1 );
    }

}