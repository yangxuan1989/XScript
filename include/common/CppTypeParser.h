﻿/**@file  		CppTypeParser.h
* @brief		Fetch type information of C++ base type
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#ifndef _XS_CPP_TYPE_H_
#define _XS_CPP_TYPE_H_

#include <type_traits>
#include <typeinfo>
#include "common/CommonType.h"

namespace XS
{
	///< type of c++
	enum EDataType
	{
		eDT_void = 0,
		eDT_char = 1,
		eDT_int8 = 2,
		eDT_int16 = 3,
		eDT_int32 = 4,
		eDT_int64 = 5,
		eDT_long = 6,
		eDT_uint8 = 7,
		eDT_uint16 = 8,
		eDT_uint32 = 9,
		eDT_uint64 = 10,
		eDT_ulong = 11,
		eDT_wchar = 12,
		eDT_bool = 13,
		eDT_float = 14,
		eDT_double = 15,
		eDT_const_char_str = 16,
		eDT_const_wchar_t_str = 17,
		eDT_enum = 18,
		eDT_class = 19,
		eDT_count = 20,
	};

	enum EDataTypeEx
	{
		eDTE_Invalid = 0,
		eDTE_Value = 1,
		eDTE_Const = 2,
		eDTE_Pointer = 3,
		eDTE_ConstPointer = 4,
		eDTE_Reference = 5,
		eDTE_Count,
	};

	template<typename T>
	struct STypeID 
	{
		enum
		{ 
			eTypeID = std::is_enum<T>::value ? eDT_enum : eDT_class,
			eSize = sizeof(T) 
		};
	};

	///< Builtin type of c++
	template<> struct STypeID<void>		{ enum{ eTypeID = eDT_void	, eSize = 0					}; };
	template<> struct STypeID<char>		{ enum{ eTypeID = eDT_char	, eSize = sizeof(char)		}; };
	template<> struct STypeID<int8>		{ enum{ eTypeID = eDT_int8	, eSize = sizeof(int8)		}; };
	template<> struct STypeID<int16>	{ enum{ eTypeID = eDT_int16	, eSize = sizeof(int16)		}; };
	template<> struct STypeID<int32>	{ enum{ eTypeID = eDT_int32	, eSize = sizeof(int32)		}; };
	template<> struct STypeID<long>		{ enum{ eTypeID = eDT_long	, eSize = sizeof(long)		}; };
	template<> struct STypeID<int64>	{ enum{ eTypeID = eDT_int64	, eSize = sizeof(int64)		}; };
	template<> struct STypeID<uint8>	{ enum{ eTypeID = eDT_uint8	, eSize = sizeof(uint8)		}; };
	template<> struct STypeID<uint16>	{ enum{ eTypeID = eDT_uint16, eSize = sizeof(uint16)	}; };
	template<> struct STypeID<uint32>	{ enum{ eTypeID = eDT_uint32, eSize = sizeof(uint32)	}; };
	template<> struct STypeID<uint64>	{ enum{ eTypeID = eDT_uint64, eSize = sizeof(uint64)	}; };
	template<> struct STypeID<ulong>	{ enum{ eTypeID = eDT_ulong	, eSize = sizeof(ulong)		}; };
	template<> struct STypeID<wchar_t>	{ enum{ eTypeID = eDT_wchar	, eSize = sizeof(wchar_t)	}; };
	template<> struct STypeID<bool>		{ enum{ eTypeID = eDT_bool	, eSize = sizeof(bool)		}; };
	template<> struct STypeID<float>	{ enum{ eTypeID = eDT_float	, eSize = sizeof(float)		}; };
	template<> struct STypeID<double>	{ enum{ eTypeID = eDT_double, eSize = sizeof(double)	}; };

	/**@struct template of type information
	* @brief Fetch typeinfo by template, default type is class, \n
	*		other types' information is fetched by template specialization
	*/
	template<typename T>
	struct TTypeInfo
	{
		typedef T Type;

		enum
		{
			m_eSize = STypeID<T>::eSize,
			m_eType = STypeID<T>::eTypeID,
			m_eTypeEx0 = eDTE_Value,
			m_eTypeEx1 = 0,
			m_eTypeEx2 = 0,
			m_eTypeEx3 = 0,
			m_eTypeEx4 = 0,
			m_eTypeEx5 = 0,
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 )|( m_eTypeEx1 << 4 )|( m_eTypeEx2 << 8 )|( m_eTypeEx3 << 12 )|( m_eTypeEx4 << 16 )|( m_eTypeEx5 << 20 ),
		};
	};

	///< Constant type
	template<typename T>
	struct TTypeInfo<const T> : public TTypeInfo<T>
	{
		enum
		{
			m_eTypeEx1 = TTypeInfo<T>::m_eTypeEx1 ? TTypeInfo<T>::m_eTypeEx1 : eDTE_Const,
			m_eTypeEx2 = TTypeInfo<T>::m_eTypeEx2 ? TTypeInfo<T>::m_eTypeEx2 : ( TTypeInfo<T>::m_eTypeEx1 ? eDTE_Const : 0 ),
			m_eTypeEx3 = TTypeInfo<T>::m_eTypeEx3 ? TTypeInfo<T>::m_eTypeEx3 : ( TTypeInfo<T>::m_eTypeEx2 ? eDTE_Const : 0 ),
			m_eTypeEx4 = TTypeInfo<T>::m_eTypeEx4 ? TTypeInfo<T>::m_eTypeEx4 : ( TTypeInfo<T>::m_eTypeEx3 ? eDTE_Const : 0 ),
			m_eTypeEx5 = TTypeInfo<T>::m_eTypeEx5 ? TTypeInfo<T>::m_eTypeEx5 : ( TTypeInfo<T>::m_eTypeEx4 ? eDTE_Const : 0 ),
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 )|( m_eTypeEx1 << 4 )|( m_eTypeEx2 << 8 )|( m_eTypeEx3 << 12 )|( m_eTypeEx4 << 16 )|( m_eTypeEx5 << 20 ),
		};
	};

	///< Pointer type
	template<typename T>
	struct TTypeInfo<T*> : public TTypeInfo<T>
	{
		enum
		{
			m_eTypeEx1 = TTypeInfo<T>::m_eTypeEx1 ? TTypeInfo<T>::m_eTypeEx1 : eDTE_Pointer,
			m_eTypeEx2 = TTypeInfo<T>::m_eTypeEx2 ? TTypeInfo<T>::m_eTypeEx2 : ( TTypeInfo<T>::m_eTypeEx1 ? eDTE_Pointer : 0 ),
			m_eTypeEx3 = TTypeInfo<T>::m_eTypeEx3 ? TTypeInfo<T>::m_eTypeEx3 : ( TTypeInfo<T>::m_eTypeEx2 ? eDTE_Pointer : 0 ),
			m_eTypeEx4 = TTypeInfo<T>::m_eTypeEx4 ? TTypeInfo<T>::m_eTypeEx4 : ( TTypeInfo<T>::m_eTypeEx3 ? eDTE_Pointer : 0 ),
			m_eTypeEx5 = TTypeInfo<T>::m_eTypeEx5 ? TTypeInfo<T>::m_eTypeEx5 : ( TTypeInfo<T>::m_eTypeEx4 ? eDTE_Pointer : 0 ),
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 )|( m_eTypeEx1 << 4 )|( m_eTypeEx2 << 8 )|( m_eTypeEx3 << 12 )|( m_eTypeEx4 << 16 )|( m_eTypeEx5 << 20 ),
		};
	};

	///< Constant pointer type
	template<typename T>
	struct TTypeInfo<T *const> : public TTypeInfo<T>
	{
		enum
		{
			m_eTypeEx1 = TTypeInfo<T>::m_eTypeEx1 ? TTypeInfo<T>::m_eTypeEx1 : eDTE_ConstPointer,
			m_eTypeEx2 = TTypeInfo<T>::m_eTypeEx2 ? TTypeInfo<T>::m_eTypeEx2 : ( TTypeInfo<T>::m_eTypeEx1 ? eDTE_ConstPointer : 0 ),
			m_eTypeEx3 = TTypeInfo<T>::m_eTypeEx3 ? TTypeInfo<T>::m_eTypeEx3 : ( TTypeInfo<T>::m_eTypeEx2 ? eDTE_ConstPointer : 0 ),
			m_eTypeEx4 = TTypeInfo<T>::m_eTypeEx4 ? TTypeInfo<T>::m_eTypeEx4 : ( TTypeInfo<T>::m_eTypeEx3 ? eDTE_ConstPointer : 0 ),
			m_eTypeEx5 = TTypeInfo<T>::m_eTypeEx5 ? TTypeInfo<T>::m_eTypeEx5 : ( TTypeInfo<T>::m_eTypeEx4 ? eDTE_ConstPointer : 0 ),
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 )|( m_eTypeEx1 << 4 )|( m_eTypeEx2 << 8 )|( m_eTypeEx3 << 12 )|( m_eTypeEx4 << 16 )|( m_eTypeEx5 << 20 ),
		};
	};

	///< Reference type
	template<typename T>
	struct TTypeInfo<T&> : public TTypeInfo<T>
	{
		enum
		{
			m_eTypeEx1 = TTypeInfo<T>::m_eTypeEx1 ? TTypeInfo<T>::m_eTypeEx1 : eDTE_Reference,
			m_eTypeEx2 = TTypeInfo<T>::m_eTypeEx2 ? TTypeInfo<T>::m_eTypeEx2 : ( TTypeInfo<T>::m_eTypeEx1 ? eDTE_Reference : 0 ),
			m_eTypeEx3 = TTypeInfo<T>::m_eTypeEx3 ? TTypeInfo<T>::m_eTypeEx3 : ( TTypeInfo<T>::m_eTypeEx2 ? eDTE_Reference : 0 ),
			m_eTypeEx4 = TTypeInfo<T>::m_eTypeEx4 ? TTypeInfo<T>::m_eTypeEx4 : ( TTypeInfo<T>::m_eTypeEx3 ? eDTE_Reference : 0 ),
			m_eTypeEx5 = TTypeInfo<T>::m_eTypeEx5 ? TTypeInfo<T>::m_eTypeEx5 : ( TTypeInfo<T>::m_eTypeEx4 ? eDTE_Reference : 0 ),
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 )|( m_eTypeEx1 << 4 )|( m_eTypeEx2 << 8 )|( m_eTypeEx3 << 12 )|( m_eTypeEx4 << 16 )|( m_eTypeEx5 << 20 ),
		};
	};

	///< Wide string type
	template<>
	struct TTypeInfo<const wchar_t*>
	{
		typedef const wchar_t* Type;

		enum
		{
			m_eSize = sizeof( const wchar_t* ),
			m_eType = eDT_const_wchar_t_str,
			m_eTypeEx0 = eDTE_Value,
			m_eTypeEx1 = 0,
			m_eTypeEx2 = 0,
			m_eTypeEx3 = 0,
			m_eTypeEx4 = 0,
			m_eTypeEx5 = 0,
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 ),
		};
	};

	///< C string type
	template<>
	struct TTypeInfo<const char*>
	{
		typedef const char* Type;

		enum
		{
			m_eSize = sizeof( const char* ),
			m_eType = eDT_const_char_str,
			m_eTypeEx0 = eDTE_Value,
			m_eTypeEx1 = 0,
			m_eTypeEx2 = 0,
			m_eTypeEx3 = 0,
			m_eTypeEx4 = 0,
			m_eTypeEx5 = 0,
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 ),
		};
	};

	///< Void type
	template<>
	struct TTypeInfo<void>
	{
		typedef const void Type;

		enum
		{
			m_eSize = 0,
			m_eType = eDT_void,
			m_eTypeEx0 = eDTE_Value,
			m_eTypeEx1 = 0,
			m_eTypeEx2 = 0,
			m_eTypeEx3 = 0,
			m_eTypeEx4 = 0,
			m_eTypeEx5 = 0,
			m_eTotalType = ( m_eType << 24 )|( m_eTypeEx0 << 0 ),
		};
	};

	///< POD reference type
	template<> struct TTypeInfo<const char		&> : public TTypeInfo<char		> {};
	template<> struct TTypeInfo<const int8		&> : public TTypeInfo<int8		> {};
	template<> struct TTypeInfo<const int16		&> : public TTypeInfo<int16		> {};
	template<> struct TTypeInfo<const int32		&> : public TTypeInfo<int32		> {};
	template<> struct TTypeInfo<const int64		&> : public TTypeInfo<int64		> {};
	template<> struct TTypeInfo<const long		&> : public TTypeInfo<long		> {};
	template<> struct TTypeInfo<const wchar_t	&> : public TTypeInfo<wchar_t	> {};
	template<> struct TTypeInfo<const uint8		&> : public TTypeInfo<uint8		> {};
	template<> struct TTypeInfo<const uint16	&> : public TTypeInfo<uint16	> {};
	template<> struct TTypeInfo<const uint32	&> : public TTypeInfo<uint32	> {};
	template<> struct TTypeInfo<const uint64	&> : public TTypeInfo<uint64	> {};
	template<> struct TTypeInfo<const ulong		&> : public TTypeInfo<ulong		> {};
	template<> struct TTypeInfo<const float		&> : public TTypeInfo<float		> {};
	template<> struct TTypeInfo<const double	&> : public TTypeInfo<double	> {};

	struct STypeInfo
	{
		uint32		m_nSize;
		uint32		m_nType;
		const char*	m_szTypeName;
	};

	/**
	* @brief Get information of the giving type
	* @return information of the giving type
	*/
	template<typename T>
	STypeInfo GetTypeInfo()
	{
		STypeInfo Info;
		Info.m_nSize = TTypeInfo<T>::m_eSize;
		Info.m_nType = TTypeInfo<T>::m_eTotalType;
		Info.m_szTypeName = typeid(typename TTypeInfo<T>::Type).name();
		return Info;
	}
}

#endif