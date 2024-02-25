#include "EventHandler.h"

#include <ArcdpsExtension/arcdps_structs.h>
#include <ArcdpsExtension/ArcdpsExtension.h>
#include <Nexus/Nexus.h>

#include <Windows.h>

// It is guaranteed that arcdps loads first and Nexus second.
// except when users do weird shit!

namespace {
	arcdps_exports arcExports{};
	AddonDefinition NexusDef{};
	AddonAPI* nexusApi = nullptr;
} // namespace

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE; // Successful DLL_PROCESS_ATTACH.
}

void mod_combat_squad(cbtevent* pEvent, ag* pSrc, ag* pDst, const char* pSkillname, uint64_t pId, uint64_t pRevision) {
	SquadEventHandler::instance().Event(pEvent, pSrc, pDst, pSkillname, pId, pRevision);
}

void mod_combat_local(cbtevent* pEvent, ag* pSrc, ag* pDst, const char* pSkillname, uint64_t pId, uint64_t pRevision) {
	LocalEventHandler::instance().Event(pEvent, pSrc, pDst, pSkillname, pId, pRevision);
}

arcdps_exports* mod_init() {
	// if this gets called before Nexus init, we don't want to be loaded!
	// Don't load and give arcdps an error message.

	arcExports.out_name = "Nexus ArcDPS Bridge";
	// hardcoded imgui version, since we don't use it!
	arcExports.imguivers = 18000;
	// TODO: generate version from VC_VERSION_INFO
	arcExports.out_build = __DATE__ " " __TIME__;

	if (nexusApi == nullptr) {
		// arcdps loaded first, we are in an error state, skip initialization and tell arcdps about it!
		std::string error_message = "Arcdps was loaded first, we do not support this loading order!";
		arcExports.sig = 0;
		const std::string::size_type size = error_message.size();
		char* buffer = new char[size + 1]; // we need extra char for NUL
		memcpy(buffer, error_message.c_str(), size + 1);
		arcExports.size = reinterpret_cast<uintptr_t>(buffer);
	} else {
		arcExports.sig = 0xFED81763; // -19392669
		arcExports.size = sizeof(arcdps_exports);
		arcExports.combat = mod_combat_squad;
		arcExports.combat_local = mod_combat_local;
	}

	return &arcExports;
}

extern "C" __declspec(dllexport) ModInitSignature get_init_addr(const char* arcversion, ImGuiContext* imguictx, void* dxptr, HMODULE arcdll, MallocSignature mallocfn, FreeSignature freefn, UINT dxver) {
	// we are not interested in anything arcdps gives us here
	return mod_init;
}

uint64_t mod_release() {
	ArcdpsExtension::Shutdown();

	return 0;
}

extern "C" __declspec(dllexport) ModReleaseSignature get_release_addr() {
	return mod_release;
}

void AddonLoad(AddonAPI* pApi) {
	// this has to be called before arcpds init!

	nexusApi = pApi;

	SquadEventHandler::instance().SetNexusApi(pApi);
	LocalEventHandler::instance().SetNexusApi(pApi);
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef() {
	NexusDef.Signature = -0x127E89D; // 0xFED81763 // -19392669
	NexusDef.APIVersion = NEXUS_API_VERSION;
	NexusDef.Name = "Nexus ArcDPS Bridge";
	// TODO: generate version from VC_VERSION_INFO
	NexusDef.Version.Major = 1;
	NexusDef.Version.Minor = 0;
	NexusDef.Version.Build = 0;
	NexusDef.Version.Revision = 1;
	NexusDef.Author = "Raidcore";
	NexusDef.Description = "Raises Nexus Events from ArcDPS combat API callbacks.";
	NexusDef.Load = AddonLoad;
	NexusDef.Flags = EAddonFlags_DisableHotloading;

	return &NexusDef;
}
