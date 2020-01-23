﻿#ifndef __DEBUG_JS_H__
#define __DEBUG_JS_H__
//=====================================================================
// CLuaDebug.h 
// 为lua定义的调试器接口
// 柯达昭
// 2007-10-16
//=====================================================================
#include "v8/v8.h"
#include "v8/v8-inspector.h"
#include "v8/v8-platform.h"
#include "common/TRBTree.h"
#include "core/CDebugBase.h"

namespace Gamma
{
	typedef v8::Local<v8::Context> CV8Context;
	typedef std::unique_ptr<v8_inspector::V8Inspector> CInspectorPtr;
	typedef std::unique_ptr<v8_inspector::V8InspectorSession> CInsSessionPtr;

	class CDebugJS
		: public CDebugBase
		, public v8_inspector::V8InspectorClient
		, public v8_inspector::V8Inspector::Channel
	{
		enum { eScopeID = 1 };
		struct SObjectInfo;
		typedef TRBTree<SObjectInfo> CObjectMap;

		struct SLocation 
		{
			int32 nScriptId = 0;
			int32 nLineNum = -1;
			int32 nColumnNum = -1;
			void ReadFromJson(CJson* pJson);
		}; 
		
		struct SObjectInfo : public CObjectMap::CRBTreeNode
		{
			std::string strID;
			std::string strType;
			std::string	strClassName;
			std::string strDesc;
			std::string strValue;
			std::vector<uint32> vecName;
			std::vector<uint32> vecIndex;
			bool bChildrenFetched = false;
			operator const std::string& () const { return strID; }
			bool operator < (const std::string& strKey) { return strID < strKey; }
			void ReadFromJson(CJson* pJson);
		};

		struct SScopeInfo
		{
			std::string strType;
			SObjectInfo* ObjectInfo;
			SLocation StartLocation;
			SLocation EndLocation;
			void ReadFromJson(CJson* pJson);
		};

		struct SObjectRefInfo
		{
			std::string strFieldName;
			SObjectInfo* pObjectInfo;
		};

		struct SFrameInfo
		{	
			uint32 nVariableID = eScopeID;
			uint32 nMaxScopeID = 0;
			std::string strCallFrameID;
			std::string	strFunctionName;
			std::string	strScriptUrl;
			SLocation FunctionLocation;
			SLocation PauseLocation;
			SObjectInfo* ThisInfo;
			std::vector<SScopeInfo> vecScope;
			std::map<uint32, SObjectRefInfo> mapObjRefs;
		};
		typedef std::vector<SFrameInfo> FrameArray;
		typedef std::map<uint32, std::string> IDStringMap;

		uint16				m_nDebugPort;
		bool				m_bChromeProtocol;
		CInspectorPtr		m_Inspector;
		CInsSessionPtr		m_Session;
		std::string			m_strUtf8Buffer;

		uint32				m_nMessageID;	
		FrameArray			m_aryFrame;
		IDStringMap			m_mapBreakPoint;
		IDStringMap			m_mapScriptInfo;
		CObjectMap			m_mapObjects;
		
		virtual bool		CheckRemoteSocket( char(&szBuffer)[2048], int32 nCurSize );
		virtual bool		ProcessCommand(CDebugCmd* pCmd);
		void				SendWebSocketData( uint8 nId, const char* pData, uint32 nSize );
		virtual uint32		GenBreakPointID(const char* szFileName, int32 nLine);
		void				ClearVariables();
		void				FetchChildren( SObjectInfo& ObjInfo );
		uint32				AddFrameObject( SFrameInfo& FrameInfo, SObjectInfo& ObjInfo, 
								std::string strField, std::string strParentID = "" );
	public:
		CDebugJS(CScriptBase* pBase, uint16 nDebugPort);
		~CDebugJS(void);

		void				AddScriptInfo(int32 nID, const char* szFileName);
		virtual void		DelBreakPoint(uint32 nBreakPointID);

		virtual uint32		GetFrameCount();
		virtual bool		GetFrameInfo(int32 nFrame, int32* nLine, const char** szFunction,
								const char** szSource);
		virtual int32		SwitchFrame(int32 nCurFrame);
		virtual uint32		EvaluateExpression(int32 nCurFrame, const char* szExpression);
		virtual uint32		GetScopeChainID(int32 nCurFrame) { return eScopeID; };
		virtual uint32		GetChildrenID(uint32 nParentID, bool bIndex,
								uint32 nStart, uint32* aryChild, uint32 nCount);
		virtual SValueInfo	GetVariable(uint32 nID);
		virtual void		Stop();
		virtual void		Continue();
		virtual void		StepIn();
		virtual void		StepNext();
		virtual void		StepOut();

		virtual void		runMessageLoopOnPause(int contextGroupId);
		virtual void		quitMessageLoopOnPause();
		virtual void		runIfWaitingForDebugger(int contextGroupId);
		virtual CV8Context	ensureDefaultContextInGroup(int context_group_id);
		virtual double		currentTimeMS();
		virtual void		sendResponse( int callId, std::unique_ptr<v8_inspector::StringBuffer> message );
		virtual void		sendNotification( std::unique_ptr<v8_inspector::StringBuffer> message );
		virtual void		flushProtocolNotifications();
	};
}

#endif
