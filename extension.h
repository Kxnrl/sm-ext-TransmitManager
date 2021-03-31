#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"

#include <extensions/ISDKHooks.h>

class TransmitManager : public SDKExtension, public ISMEntityListener, public IClientListener
{

public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
    virtual void SDK_OnUnload();
	virtual bool QueryRunning(char* error, size_t maxlength);
	virtual void NotifyInterfaceDrop(SMInterface* pInterface);

	virtual void OnEntityDestroyed(CBaseEntity* pEntity);

	virtual void OnClientPutInServer(int client);
	virtual void OnClientDisconnecting(int client);

	void Hook_SetTransmit(CCheckTransmitInfo* pInfo, bool bAlways);

	void HookEntity(CBaseEntity* pEntity);

private:
	void UnhookEntity(int index);
};

inline bool IsEntityIndexInRange(int i) { return i >= 1 && i < MAX_EDICTS; }

#endif
