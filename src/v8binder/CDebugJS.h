﻿#ifndef __DEBUG_JS_H__
#define __DEBUG_JS_H__
//=====================================================================
// CLuaDebug.h 
// 为lua定义的调试器接口
// 柯达昭
// 2007-10-16
//=====================================================================

#include "v8/v8-inspector.h"
#include "v8/v8-platform.h"
#include "core/CDebugBase.h"
using namespace std;

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
		uint16				m_nDebugPort;
		bool				m_bLoopOnPause;
		CInspectorPtr		m_Inspector;
		CInsSessionPtr		m_Session;
		string				m_strUtf8Buffer;

		virtual bool		CheckRemoteSocket( char(&szBuffer)[2048], int32 nCurSize );
		virtual bool		ProcessCommand(CDebugCmd* pCmd);
		void				SendWebSocketData( uint8 nId, const char* pData, uint32 nSize );
		virtual uint32		GenBreakPointID(const char* szFileName, int32 nLine);
	public:
		CDebugJS(CScriptBase* pBase, uint16 nDebugPort);
		~CDebugJS(void);

		void				Break();

		virtual void		DelBreakPoint(uint32 nBreakPointID);;

		virtual uint32		GetFrameCount();
		virtual bool		GetFrameInfo(int32 nFrame, int32* nLine, const char** szFunction,
								const char** szSource);
		virtual int32		SwitchFrame(int32 nCurFrame);
		virtual uint32		GetVariableID(int32 nCurFrame, const char* szName);
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