#include <iostream>

#include "ArcdpsEventHandler.h"
#include "ArcdpsExtension/ArcdpsExtension.h"
#include "ArcdpsExtension/arcdps_structs.h"
#include "Nexus/Nexus.h"

namespace {
    HMODULE SELF_DLL;
    ID3D11Device* d3d11Device = nullptr;

    EventSequencer localEventHandler("EV_ARC_COMBATEVENT_LOCAL");
    EventSequencer areaEventHandler("EV_ARC_COMBATEVENT_SQUAD");
}

BOOL APIENTRY DllMain(HMODULE pModule,
                      DWORD pReasonForCall,
                      LPVOID pReserved
) {
    switch (pReasonForCall) {
        case DLL_PROCESS_ATTACH:
            SELF_DLL = pModule;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

// ARCDPS EXPORTS
namespace {
    arcdps_exports arc_exports = {};
}

uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision) {
    areaEventHandler.ProcessEvent(ev, src, dst, skillname, id, revision);
    return 0;
}

uintptr_t mod_combat_local(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision) {
    areaEventHandler.ProcessEvent(ev, src, dst, skillname, id, revision);
    return 0;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init() {
    arc_exports.imguivers = IMGUI_VERSION_NUM;
    arc_exports.out_name = "Nexus ArcDPS Bridge";
    arc_exports.out_build = __DATE__ " " __TIME__;
    arc_exports.sig = 0x2e574f9d;
    arc_exports.size = sizeof(arcdps_exports);
    // arc_exports.wnd_nofilter = // do we need this?
    arc_exports.combat = mod_combat; // combat events to share
    arc_exports.combat_local = mod_combat_local; // combat events from chatlog to share
    // arc_exports.imgui = // imgui callback for rendering. Should it be shared?
    // arc_exports.options_end = // imgui callback for arcdps options. Should this be shared?
    // arc_exports.options_windows = // imgui callback to draw arcdps checkboxes. Should this be shared?
    return &arc_exports;
}


/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, ImGuiContext* imguicontext, IDXGISwapChain* dxptr,
                                                     HMODULE new_arcdll, void* mallocfn,
                                                     void* freefn, UINT dxver) {
    dxptr->GetDevice(__uuidof(d3d11Device), reinterpret_cast<void**>(&d3d11Device));
//    ArcdpsExtension::Setup(SELF_DLL, d3d11Device, imguicontext);
    return mod_init;
}

uintptr_t mod_release() {
    ArcdpsExtension::Shutdown();
    return 0;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr() {
    return mod_release;
}

// NEXUS EXPORTS
namespace {
    AddonDefinition addon_def = {};
    AddonAPI* NEXUS_API = nullptr;
}

void AddonLoad(AddonAPI* pApi)
{
    NEXUS_API = pApi;
    localEventHandler.SetAddonApi(pApi);
    areaEventHandler.SetAddonApi(pApi);
}

void AddonUnload()
{
    return;
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
    addon_def.Signature = -19392670;
    addon_def.APIVersion = NEXUS_API_VERSION;
    addon_def.Name = "Nexus ArcDPS Bridge";
    addon_def.Version.Major = 1;
    addon_def.Version.Minor = 0;
    addon_def.Version.Build = 0;
    addon_def.Version.Revision = 1;
    addon_def.Author = "knoxfighter";
    addon_def.Description = "Adds events for the ArcDPS combat API.";
    addon_def.Load = AddonLoad;
    addon_def.Unload = AddonUnload;

    return &addon_def;
}
