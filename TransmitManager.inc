#if defined _TransmitManager_included
  #endinput
#endif
#define _TransmitManager_included

native void TransmitManager_AddEntityHooks(int entity);
native bool TransmitManager_SetEntityOwner(int entity, int target);
native bool TransmitManager_SetEntityState(int entity, int client, bool can);
native bool TransmitManager_GetEntityState(int entity, int client);
native bool TransmitManager_IsEntityHooked(int entity);

/**
 * _________________Do not edit below this line!_______________________
 */
public Extension __ext_transmit = 
{
	name = "TransmitManager",
	file = "TransmitManager.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

