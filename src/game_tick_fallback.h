/* Callback-slot fallback adapted from Beelzebub's Armor Transmog 0.1.70
 * (source/tm_entry.c), supplied by the user. Uses HellForge's shared hook chain;
 * creates no operating-system thread. Build 25480438 only.
 */
#ifndef HADE_GAME_TICK_FALLBACK_H
#define HADE_GAME_TICK_FALLBACK_H
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "hellforge_mod.h"
static const HfApi* gtApi;
static HfMod* gtMod;
static HfTickCallback gtCallback;
static void* gtUser;
#define FRAME_UPDATE_RVA 0x13270B0u
#define FRAME_CALL_RVA 0xAB4BE7u
typedef void (*FrameUpdateFn)(void* manager, float dt);
static FrameUpdateFn frameOriginal;
static uintptr_t frameReturn;
/* 0.1.70: when other mods hook the same function, HellForge's chain calls this detour from its own code, so the return
 * address is not the caller's. The game's call still left it on this thread's stack a few frames up, so the stack
 * memory is searched for it (walking the frames, 0.1.69, stops at HellForge's chain code). On some setups only the
 * function's second caller (0x13F8938) reaches it: Frame then runs from that one, when the frame update has not called
 * for 250 ms. Either way, once per frame. */
#define FRAME_CALL2_RVA 0x13F8938u
static uintptr_t frameReturn2;
static ULONGLONG frameFirstAt;
#ifndef HADE_TICK_TEST
static int OnStack(uintptr_t from, uintptr_t want)
{
    if (!want) return 0;
    if (from == want) return 1;
    volatile uintptr_t here = 0;
    uintptr_t at = ((uintptr_t)&here + 7) & ~(uintptr_t)7;
    uintptr_t top = (uintptr_t)((NT_TIB*)NtCurrentTeb())->StackBase, end = at + 0x2000;
    if (!top || top < at) return 0;
    if (end > top) end = top;
    for (; at + sizeof(uintptr_t) <= end; at += sizeof(uintptr_t))
        if (*(const uintptr_t*)at == want) return 1;
    return 0;
}
#else
static int OnStack(uintptr_t from,uintptr_t want){return from==want;}
#endif
#ifndef HADE_TICK_NOW
#define HADE_TICK_NOW GetTickCount64
#endif
static int FromFrameUpdate(uintptr_t from)
{
    ULONGLONG now = HADE_TICK_NOW();
    if (OnStack(from, frameReturn)) { frameFirstAt = now; return 1; }
    return now - frameFirstAt > 250 && OnStack(from, frameReturn2);
}
static void FrameUpdateDetour(void* manager, float dt)
{
    static unsigned calls, runs;
    static int told;
    uintptr_t from = (uintptr_t)__builtin_return_address(0);
    frameOriginal(manager, dt);
    calls++;
    if (FromFrameUpdate(from)) { runs++; gtCallback(gtMod, dt, gtUser); }
    if (!told && calls >= 600)
    {
        told = 1;
        if (runs) gtApi->log(gtMod, "game thread: the own hook runs the mod once per frame (%u of %u calls)", runs, calls);
        else gtApi->log(gtMod, "game thread: the own hook was called %u times but never from the frame update (game.dll+0x%X) or its "
                           "second caller (+0x%X); the mod does not run. Please send this log", calls, FRAME_CALL_RVA, FRAME_CALL2_RVA);
    }
}
static int HookFrameUpdate(void)
{
    size_t imageSize=0;
    uint8_t* game = (uint8_t*)gtApi->module_base("game.dll",&imageSize);
    if(imageSize!=0x4744000u){gtApi->log(gtMod,"game thread: unsupported game image; fallback refused");return 0;}
    if (!game) return 0;
    uint8_t* call = game + FRAME_CALL_RVA;
    int32_t rel;
    memcpy(&rel, call + 1, 4);
    if (call[0] != 0xE8 || call + 5 + rel != game + FRAME_UPDATE_RVA)
    {
        gtApi->log(gtMod, "game thread: the frame update's call at game.dll+0x%X is not where this build has it; not hooked", FRAME_CALL_RVA);
        return 0;
    }
    frameReturn = (uintptr_t)(call + 5);
    uint8_t* call2 = game + FRAME_CALL2_RVA;
    int32_t rel2;
    memcpy(&rel2, call2 + 1, 4);
    frameReturn2 = call2[0] == 0xE8 && call2 + 5 + rel2 == game + FRAME_UPDATE_RVA ? (uintptr_t)(call2 + 5) : 0;
    if (gtApi->hook(gtMod, (uintptr_t)(game + FRAME_UPDATE_RVA), (void*)FrameUpdateDetour, (void**)&frameOriginal) != 0 || !frameOriginal)
    {
        gtApi->log(gtMod, "game thread: game.dll+0x%X could not be hooked (modloader.log says why)", FRAME_UPDATE_RVA);
        return 0;
    }
    gtApi->log(gtMod, "game thread: HellForge's 16 game-thread callbacks are all taken; the mod runs from its own hook on the per-frame "
                  "update (game.dll+0x%X, a shared hook)", FRAME_UPDATE_RVA);
    return 1;
}


static void GtGameLoaded(HfMod* mod,HfEvent event,void* user)
{
    if(!HookFrameUpdate())gtApi->log(mod,"game thread: fallback unavailable; mod tick disabled; please send modloader.log and this mod's log");
}
static int HadeGameTickRegister(const HfApi* api,HfMod* mod,HfTickCallback callback,void* user)
{
    if(api->on_game_thread(mod,callback,user))return 1;
    gtApi=api;gtMod=mod;gtCallback=callback;gtUser=user;
    api->log(mod,"game thread: HellForge callback registration refused; attempting validated shared-hook fallback (see modloader.log)");
    size_t imageSize=0;
    if(api->module_base("game.dll",&imageSize))return HookFrameUpdate();
    /* At initial load game.dll may not exist yet. Install only after it loads. */
    return api->on(mod,HF_EVENT_GAME_LOADED,GtGameLoaded,NULL);
}
#endif
