#include "extension.h"

#ifdef _WINDOWS
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

constexpr auto MAX_EDICT = 2048;

TransmitManager g_TransmitManager;
SMEXT_LINK(&g_TransmitManager);

IGameConfig* g_pGameConf = nullptr;
ISDKHooks* g_pSDKHooks = nullptr;
extern sp_nativeinfo_t g_Natives[];

SH_DECL_MANUALHOOK2_void(SetTransmit, 0, 0, 0, CCheckTransmitInfo*, bool);

struct HookingEntity
{
    HookingEntity(CBaseEntity *pEntity)
    {
        bRemoveFlags = false;
        iOwnerEntity = -1;
        iEntityIndex = gamehelpers->EntityToBCompatRef(pEntity);
        SourceHookId = SH_ADD_MANUALHOOK(SetTransmit, pEntity, SH_MEMBER(&g_TransmitManager, &TransmitManager::Hook_SetTransmit), false);

        for (auto i = 1; i < SM_MAXPLAYERS; i++)
        {
            // can see by default
            bCanTransmit[i] = true;
        }

        auto *pName = gamehelpers->GetEntityClassname(pEntity);
        if (pName != nullptr && (V_strcmp(pName, "info_particle_system") == 0 || V_strcmp(pName, "light_dynamic") == 0))
        {
            bRemoveFlags = true;

            auto *edict = gamehelpers->EdictOfIndex(iEntityIndex);
            if (edict)
            {
                auto flags = edict->m_fStateFlags;
                edict->m_fStateFlags = flags & (~FL_EDICT_ALWAYS);
            }
        }
    }

    ~HookingEntity()
    {
        if (SourceHookId)
        {
            SH_REMOVE_HOOK_ID(SourceHookId);
            SourceHookId = 0;
        }
    }

    bool CanSee(int client)
    {
        if (iEntityIndex == client || iOwnerEntity == client)
        {
            // self or children
            return true;
        }

        return bCanTransmit[client];
    }

    void SetSee(int client, bool can)
    {
        bCanTransmit[client] = can;
    }

    int GetOwner()
    {
        return iOwnerEntity;
    }

    void SetOwner(int owner)
    {
        iOwnerEntity = owner;
    }

    bool ShouldRemove()
    {
        return bRemoveFlags;
    }

private:
    int SourceHookId;
    int iEntityIndex;
    bool bCanTransmit[SM_MAXPLAYERS];
    int iOwnerEntity;
    bool bRemoveFlags;
};

HookingEntity* g_Hooked[MAX_EDICT];

void TransmitManager::Hook_SetTransmit(CCheckTransmitInfo* pInfo, bool bAlways)
{
    auto *bcref = META_IFACEPTR(CBaseEntity);
    auto entity = gamehelpers->EntityToBCompatRef(bcref);
    auto *edict = gamehelpers->EdictOfIndex(entity);
    auto client = gamehelpers->IndexOfEdict(pInfo->m_pClientEnt);

    if (entity > MAX_EDICT || entity < 1 || client == entity || !edict || edict->IsFree())
    {
        RETURN_META(MRES_IGNORED);
    }

    if (g_Hooked[entity]->ShouldRemove())
    {
        auto flags = edict->m_fStateFlags;
        if (flags & FL_EDICT_ALWAYS)
        {
            edict->m_fStateFlags = (flags ^ FL_EDICT_ALWAYS);
        }
    }

    auto owner = g_Hooked[entity]->GetOwner();
    if (owner == client)
    {
        // don't block children
        RETURN_META(MRES_IGNORED);
    }

    if (!g_Hooked[entity]->CanSee(client))
    {
        // blocked
        RETURN_META(MRES_SUPERCEDE);
    }

    if (IsEntityIndexInRange(owner) && !g_Hooked[owner]->CanSee(client))
    {
        // blocked
        RETURN_META(MRES_SUPERCEDE);
    }

    //META_CONPRINTF("Hook_SetTransmit-> %d | %d | %d | %s\n", entity, owner, client, g_Hooked[entity]->CanSee(client) ? "true" : "false");

    RETURN_META(MRES_IGNORED);
}

bool TransmitManager::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
    sharesys->AddDependency(myself, "sdkhooks.ext", true, true);
    if (!sharesys->RequestInterface(SMINTERFACE_SDKHOOKS_NAME, SMINTERFACE_SDKHOOKS_VERSION, myself, reinterpret_cast<SMInterface**>(&g_pSDKHooks))) {
        smutils->Format(error, maxlen, "Cannot get SDKHooks Interface");
        return false;
    }

    if (!gameconfs->LoadGameConfigFile("sdkhooks.games", &g_pGameConf, error, maxlen)) {
        smutils->Format(error, maxlen, "Failed to load SDKHooks gamedata.");
        return false;
    }

    int offset = -1;
    if (!g_pGameConf->GetOffset("SetTransmit", &offset)) {
        smutils->Format(error, maxlen, "Failed to load 'SetTransmit' offset.");
        return false;
    }
    SH_MANUALHOOK_RECONFIGURE(SetTransmit, offset, 0, 0);

    playerhelpers->AddClientListener(this);
    g_pSDKHooks->AddEntityListener(this);

    g_pShareSys->AddNatives(myself, g_Natives);
    g_pShareSys->RegisterLibrary(myself, "TransmitManager");

    // base
    g_Hooked[0] = nullptr;

    return true;
}

void TransmitManager::SDK_OnUnload()
{
    playerhelpers->RemoveClientListener(this);
    g_pSDKHooks->RemoveEntityListener(this);

    for (auto i = 0; i < MAX_EDICT; i++)
    {
        if (g_Hooked[i] != nullptr)
        {
            delete g_Hooked[i];
        }
    }
}

void TransmitManager::OnEntityDestroyed(CBaseEntity* pEntity)
{
    if (!pEntity)
    {
        smutils->LogError(myself, "OnEntityDestroyed -> nullptr detected...");
        return;
    }

    auto entRef = gamehelpers->EntityToReference(pEntity);
    auto entity = gamehelpers->ReferenceToIndex(entRef);

    if ((unsigned)entity == INVALID_EHANDLE_INDEX || (entity > 0 && entity <= playerhelpers->GetMaxClients()))
    {
        // This can be -1 for player entity before any players have connected
        return;
    }

    if (!IsEntityIndexInRange(entity))
    {
        // out-of-range
        return;
    }

    UnhookEntity(entity);
}

void TransmitManager::OnClientPutInServer(int client)
{
    auto *pPlayer = playerhelpers->GetGamePlayer(client);
    if (!pPlayer || pPlayer->IsFakeClient())
    {
        return;
    }
  
    for (auto i = 1; i < MAX_EDICT; i++)
    {
        if (g_Hooked[i] == nullptr)
        {
            //not being hook
            continue;
        }

        g_Hooked[i]->SetSee(client, true);
    }
}

void TransmitManager::OnClientDisconnecting(int client)
{
    auto* pPlayer = playerhelpers->GetGamePlayer(client);
    if (!pPlayer || pPlayer->IsFakeClient())
    {
        // not in-game = no hook
        return;
    }

    UnhookEntity(client);
}

void TransmitManager::HookEntity(CBaseEntity* pEntity)
{
    auto index = gamehelpers->EntityToBCompatRef(pEntity);

    if (!IsEntityIndexInRange(index))
    {
        // out-of-range
        smutils->LogError(myself, "Failed to hook entity %d -> out-of-range.", index);
        return;
    }

    if (g_Hooked[index] != nullptr)
    {
        smutils->LogError(myself, "Entity Hook listener [%d] is not nullptr. Try to remove.", index);
        delete g_Hooked[index];
    }

    g_Hooked[index] = new HookingEntity(pEntity);
    //smutils->LogMessage(myself, "Hooked entity %d", index);
}

void TransmitManager::UnhookEntity(int index)
{
    if (!IsEntityIndexInRange(index))
    {
        // out-of-range
        smutils->LogError(myself, "Failed to unhook entity %d -> out-of-range.", index);
        return;
    }

    if (g_Hooked[index] == nullptr)
    {
        //smutils->LogError(myself, "Entity Hook listener %d is nullptr. Skipped.", index);
        return;
    }

    delete g_Hooked[index];
    g_Hooked[index] = nullptr;
    //smutils->LogMessage(myself, "Unhooked entity %d", index);
}

static cell_t Native_SetEntityOwner(IPluginContext* pContext, const cell_t* params)
{
    if (!(params[1] >= 1 && params[1] < MAX_EDICT))
    {
        // out-of-range
        return pContext->ThrowNativeError("Entity %d is out-of-range.", params[1]);
    }

    if (g_Hooked[params[1]] != nullptr)
    {
        g_Hooked[params[1]]->SetOwner(params[2]);
        return true;
    }
        
    return pContext->ThrowNativeError("Entity %d is not being hook.", params[1]);
}

static cell_t Native_SetEntityState(IPluginContext* pContext, const cell_t* params)
{
    if (!(params[1] >= 1 && params[1] < MAX_EDICT))
    {
        // out-of-range
        return pContext->ThrowNativeError("Entity %d is out-of-range.", params[1]);
    }

    auto *pPlayer = playerhelpers->GetGamePlayer(params[2]);
    if (!pPlayer || !pPlayer->IsInGame())
    {
        return pContext->ThrowNativeError("Client %d is invalid.", params[2]);
    }

    if (g_Hooked[params[1]] != nullptr)
    {
        g_Hooked[params[1]]->SetSee(params[2], params[3]);
        return true;
    }

    return pContext->ThrowNativeError("Entity %d is not being hook.", params[1]);
}

static cell_t Native_AddEntityHooks(IPluginContext* pContext, const cell_t* params)
{
    if (!(params[1] >= 1 && params[1] < MAX_EDICT))
    {
        // out-of-range
        return pContext->ThrowNativeError("Entity %d is out-of-range.", params[1]);
    }

    auto *pEntity = gamehelpers->ReferenceToEntity(params[1]);
    if (!pEntity)
    {
        // nuull
        return pContext->ThrowNativeError("Entity %d is invalid.", params[1]);
    }

    g_TransmitManager.HookEntity(pEntity);

    return 0;
}

sp_nativeinfo_t g_Natives[] =
{
    {"TransmitManager_AddEntityHooks", Native_AddEntityHooks},
    {"TransmitManager_SetEntityOwner", Native_SetEntityOwner},
    {"TransmitManager_SetEntityState", Native_SetEntityState},
    {nullptr, nullptr},
};