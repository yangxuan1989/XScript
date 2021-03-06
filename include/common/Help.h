﻿/**@file  		Help.h
* @brief		Common macros and functions
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#ifndef __XS_HELP_H__
#define __XS_HELP_H__

#include "common/CommonType.h"
#include <assert.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DEL_GROUP
#undef SUCCEEDED
#undef FAILED
#undef MAKE_DWORD

#define SAFE_RELEASE( p )				{ if( p ){ (p)->Release(); (p) = NULL; } }
#define SAFE_DELETE( p )				{ delete (p); (p) = NULL; }
#define SAFE_DEL_GROUP( p )				{ delete[] (p); (p) = NULL; }
#define ELEM_COUNT( _array )			( (uint32)( sizeof( _array )/sizeof( _array[0] ) ) )

#define INVALID_64BITID		0xffffffffffffffffULL
#define INVALID_32BITID		0xffffffff
#define INVALID_16BITID		0xffff
#define INVALID_8BITID		0xff

#define MAX_INT64			((int64)0x7fffffffffffffffLL)
#define MIN_INT64			((int64)0x8000000000000000LL)
#define MAX_INT32			((int32)0x7fffffff)
#define MIN_INT32			((int32)0x80000000)
#define MAX_INT16			((int16)0x7fff)
#define MIN_INT16			((int16)0x8000)
#define MAX_INT8			((int8)0x7f)
#define MIN_INT8			((int8)0x80)

#define MAKE_DWORD(ch0, ch1, ch2, ch3)                            \
	((uint32)(uint8)(int8)(ch0) | ((uint32)(uint8)(int8)(ch1) << 8) |       \
	((uint32)(uint8)(int8)(ch2) << 16) | ((uint32)(uint8)(int8)(ch3) << 24 ))

#define MAKE_UINT64( ch0, ch1 ) ((uint64)(uint32)(int32)(ch0) | ((uint64)(uint32)(int32)(ch1) << 32) )
#define MAKE_UINT32( ch0, ch1 ) ((uint32)(uint16)(int16)(ch0) | ((uint32)(uint16)(int16)(ch1) << 16) )
#define MAKE_UINT16( ch0, ch1 ) ((uint16)(uint8)(int8)(ch0) | ((uint16)(uint8)(int8)(ch1) << 8) )

#define HIUINT32(l)	((uint32)((uint64)((int64)(l)) >> 32))
#define LOUINT32(l)	((uint32)((uint64)((int64)(l)) & 0xffffffff))
#define HIUINT16(l)	((uint16)((uint32)((int32)(l)) >> 16))
#define LOUINT16(l)	((uint16)((uint32)((int32)(l)) & 0xffff))
#define HIUINT8(w)	((uint8)((uint16)((int16)(w)) >> 8))
#define LOUINT8(w)	((uint8)((uint16)((int16)(w)) & 0xff))	
#define HIINT32(l)	((int32)((uint64)((int64)(l)) >> 32))
#define LOINT32(l)	((int32)((uint64)((int64)(l)) & 0xffffffff))
#define HIINT16(l)	((int16)((uint32)((int32)(l)) >> 16))
#define LOINT16(l)	((int16)((uint32)((int32)(l)) & 0xffff))
#define HIINT8(w)	((int8)((uint16)((int16)(w)) >> 8))
#define LOINT8(w)	((int8)((uint16)((int16)(w)) & 0xff))

namespace XS
{	
	/** @brief Return the left most integer value not less than n
	 *  and which modulus on nAligen is zero
	 */
	inline uint32 AligenUp( uint32 n, uint32 nAligen )
	{
		return n ? ( ( n - 1 )/nAligen + 1 )*nAligen : 0;
	}

	/** @brief Return the right most integer value not greater than n
	 *  and which modulus on nAligen is zero
	 */
	inline uint32 AligenDown( uint32 n, uint32 nAligen )
	{
		return ( n / nAligen )*nAligen;
	}

	template<class CharType>
	inline bool IsLetter( CharType c )
	{
		return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
	}

	template<class CharType>
	inline bool IsWordChar( CharType c )
	{
		return IsLetter( c ) || ( (uint32)c ) > 127;
	}

	template<class CharType>
	inline bool IsNumber( CharType c )
	{
		return c >= '0' && c <= '9';
	}

	///< Convert hex character to integer
	template<class CharType>
	inline int ValueFromHexNumber( CharType c )
	{
		if( c >= '0' && c <= '9' )
			return c - '0';
		if( c >= 'A' && c <= 'F' )
			return c - 'A' + 10;
		if( c >= 'a' && c <= 'f' )
			return c - 'a' + 10;
		return -1;
	}

	///< Convert integer to hex character
	inline int ValueToHexNumber( int n )
	{
		assert( n >= 0 && n <= 0xf );
		if( n < 10 )
			return '0' + n;
		return n - 10 + 'A';
	}

	template<class CharType>
	inline bool IsHexNumber( CharType c )
	{
		return ValueFromHexNumber( c ) >= 0;
	}

	template<class CharType>
	inline bool IsBlank( CharType c )
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}
	
	///< Remove . and .. in file path
	template<class CharType>
	inline uint32 ShortPath( CharType* szPath )
	{
		uint32 nPos[256];
		uint32 nCount = 0;
		uint32 nCurPos = 0;
		uint32 nPrePos = 0;

		for( uint32 i = 0; szPath[i]; i++ )
		{
			if( szPath[i] == '.' && ( szPath[i+1] == '/' || szPath[i+1] == '\\' ) )
			{
				i += 1;
				continue;
			}
			else if( szPath[i] == '.' && szPath[i+1] == '.' && ( szPath[i+2] == '/' || szPath[i+2] == '\\' ) )
			{
				i += 2;
				if( nCount )
				{
					nCurPos = nPos[--nCount];
					nPrePos = nCurPos;
					continue;
				}
				else
				{
					szPath[nCurPos++] = '.';
					szPath[nCurPos++] = '.';
					szPath[nCurPos++] = '/';
					nPrePos = nCurPos;
					continue;
				}
			}

			if( szPath[i] == '/' || szPath[i] == '\\' )
			{
				szPath[nCurPos++] = '/';
				nPos[nCount++] = nPrePos;
				nPrePos = nCurPos;
			}
			else
			{
				szPath[nCurPos++] = szPath[i];
			}
		}

		szPath[nCurPos] = 0;
		return (int32)nCurPos;
	}

	template<class CharType>
	CharType* GetFileNameFromPath( CharType* szPath )
	{
		size_t nPos = 0;
		for( size_t i = 0; szPath[i]; i++ )
			if( szPath[i] == '/' || szPath[i] == '\\' )
				nPos = i + 1;
		return szPath + nPos;
	}

	/// conver string to number
	int32 ToInt32( const wchar_t* szStr );
	int32 ToInt32( const char* szStr );
	int64 ToInt64( const wchar_t* szStr );
	int64 ToInt64( const char* szStr );
	double ToFloat( const wchar_t* szStr );
	double ToFloat( const char* szStr );
}
#endif
