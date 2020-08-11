#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"

#include <extensions/ISDKHooks.h>

class TransmitManager : public SDKExtension, public ISMEntityListener, public IClientListener
{

public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
    virtual void SDK_OnUnload();

public:
	virtual void OnEntityDestroyed(CBaseEntity* pEntity);

public:
	virtual void OnClientPutInServer(int client);
	virtual void OnClientDisconnecting(int client);

public:
	void Hook_SetTransmit(CCheckTransmitInfo* pInfo, bool bAlways);

private:
	inline bool IsEntityIndexInRange(int i) { return i >= 1 && i < 2048; }

public:
	void HookEntity(CBaseEntity* pEntity);
	void UnhookEntity(CBaseEntity* pEntity);

};

#endif
