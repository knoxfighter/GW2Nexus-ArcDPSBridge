#include <Windows.h>
#include <vector>
#include <string>
#include <mutex>

#include "Nexus.h"
#include "ArcDPS.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

struct EvCombatData
{
	cbtevent* ev;
	ag* src;
	ag* dst;
	char* skillname;
	uint64_t id;
	uint64_t revision;
};

/* proto / globals */
void AddonLoad(AddonAPI* aApi);
void AddonUnload();

AddonDefinition* AddonDef = nullptr;
AddonAPI* APIDefs = nullptr;

std::mutex Mutex;
std::vector<EvCombatData*> evCombatEvents;

arcdps_exports arc_exports;
uint32_t cbtcount = 0;
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversion, ImGuiContext* imguictx, void* id3dptr, HANDLE arcdll, void* mallocfn, void* freefn, uint32_t d3dversion);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_combat_squad(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision);
uintptr_t mod_combat_local(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision);
uintptr_t mod_combat(const char* channel, cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision);

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef = new AddonDefinition();
	AddonDef->Signature = -19392669;
	AddonDef->APIVersion = NEXUS_API_VERSION;
	AddonDef->Name = "Nexus ArcDPS Bridge";
	AddonDef->Version.Major = 1;
	AddonDef->Version.Minor = 0;
	AddonDef->Version.Build = 0;
	AddonDef->Version.Revision = 1;
	AddonDef->Author = "Raidcore";
	AddonDef->Description = "Adds events for the ArcDPS combat API.";
	AddonDef->Load = AddonLoad;
	AddonDef->Unload = AddonUnload;

	return AddonDef;
}

void AddonLoad(AddonAPI* aApi)
{
	APIDefs = aApi;
}

void AddonUnload()
{
	return;
}

extern "C" __declspec(dllexport) void* get_init_addr(char* arcversion, ImGuiContext* imguictx, void* id3dptr, HANDLE arcdll, void* mallocfn, void* freefn, uint32_t d3dversion)
{
	return mod_init;
}

extern "C" __declspec(dllexport) void* get_release_addr()
{
	return mod_release;
}

arcdps_exports* mod_init()
{
	memset(&arc_exports, 0, sizeof(arcdps_exports));
	arc_exports.sig = -19392669;
	arc_exports.imguivers = IMGUI_VERSION_NUM;
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = "Nexus ArcDPS Bridge";
	arc_exports.out_build = __DATE__ " " __TIME__;
	arc_exports.combat = mod_combat_squad;
	arc_exports.combat_local = mod_combat_local;

	return &arc_exports;
}

uintptr_t mod_release()
{
	return 0;
}

uintptr_t mod_combat_squad(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision)
{
	return mod_combat("squad", ev, src, dst, skillname, id, revision);
}

uintptr_t mod_combat_local(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision)
{
	return mod_combat("local", ev, src, dst, skillname, id, revision);
}

uintptr_t mod_combat(const char* channel, cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision)
{
	/*Mutex.lock();
	if (evCombatEvents.size() > 1000)
	{
		for (size_t i = 0; i < 500; i++)
		{
			if (evCombatEvents[i]->src && evCombatEvents[i]->src->name)
			{
				delete[] evCombatEvents[i]->src->name;
			}
			if (evCombatEvents[i]->dst && evCombatEvents[i]->dst->name)
			{
				delete[] evCombatEvents[i]->dst->name;
			}
			if (evCombatEvents[i]->skillname)
			{
				delete[] evCombatEvents[i]->skillname;
			}
			if (evCombatEvents[i]->src)
			{
				delete evCombatEvents[i]->src;
			}
			if (evCombatEvents[i]->dst)
			{
				delete evCombatEvents[i]->dst;
			}
			delete evCombatEvents[i];
		}

		for (size_t i = 0; i < 500; i++)
		{
			evCombatEvents.erase(evCombatEvents.begin());
		}
	}
	Mutex.unlock();*/

	EvCombatData* evCbtData = new EvCombatData();
	evCbtData->ev = ev != nullptr ? new cbtevent(*ev) : nullptr;
	evCbtData->src = src != nullptr ? new ag(*src) : nullptr;
	if (evCbtData->src && evCbtData->src->name)
	{
		size_t szSrcName = strlen(src->name) + 1;
		if (szSrcName > 1)
		{
			char* sSrcName = new char[szSrcName];
			strcpy_s(sSrcName, szSrcName, src->name);
			evCbtData->src->name = sSrcName;
		}
		else
		{
			evCbtData->src->name = nullptr;
		}
	}
	evCbtData->dst = dst != nullptr ? new ag(*dst) : nullptr;
	if (evCbtData->dst && evCbtData->dst->name)
	{
		size_t szDstName = strlen(dst->name) + 1;
		if (szDstName > 1)
		{
			char* sDstName = new char[szDstName];
			strcpy_s(sDstName, szDstName, dst->name);
			evCbtData->dst->name = sDstName;
		}
		else
		{
			evCbtData->dst->name = nullptr;
		}
	}
	if (skillname)
	{
		size_t szSkName = strlen(skillname) + 1;
		if (szSkName > 1)
		{
			char* sSkName = new char[szSkName];
			strcpy_s(sSkName, szSkName, skillname);
			evCbtData->skillname = sSkName;
		}
		else
		{
			evCbtData->skillname = nullptr;
		}
	}
	evCbtData->id = id;
	evCbtData->revision = revision;

	Mutex.lock();
	evCombatEvents.push_back(evCbtData);
	Mutex.unlock();

	if (APIDefs != nullptr)
	{
		if (strcmp(channel, "local") == 0)
		{
			APIDefs->RaiseEvent("EV_ARC_COMBATEVENT_LOCAL", evCbtData);
		}
		else
		{
			APIDefs->RaiseEvent("EV_ARC_COMBATEVENT_SQUAD", evCbtData);
		}
	}

	return 0;
}