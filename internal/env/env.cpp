#include <internal/env/env.hpp>

#include <internal/env/libs/http.hpp>
#include <internal/env/libs/closures.hpp>
#include <internal/env/libs/miscellaneous.hpp>
#include <internal/env/libs/script.hpp>
#include "libs/debug.hpp"
#include "libs/metatable.hpp"
#include "libs/cache.hpp"
#include "libs/instance.hpp"
#include "libs/signals.hpp"
#include "libs/crypt.hpp"
#include "libs/system.hpp"
#include "libs/websocket.hpp"

namespace Environment {
    std::vector<Closure*> function_array;
}

lua_CFunction OriginalIndex;
lua_CFunction OriginalNamecall;

std::vector<const char*> UnsafeFunction = {
    "TestService.Run", "TestService", "Run",
    "OpenVideosFolder", "OpenScreenshotsFolder", "GetRobuxBalance", "PerformPurchase",
    "PromptBundlePurchase", "PromptNativePurchase", "PromptProductPurchase", "PromptPurchase",
    "PromptThirdPartyPurchase", "Publish", "GetMessageId", "OpenBrowserWindow", "RequestInternal",
    "ExecuteJavaScript", "ToggleRecording", "TakeScreenshot", "HttpRequestAsync", "GetLast",
    "SendCommand", "GetAsync", "GetAsyncFullUrl", "RequestAsync", "MakeRequest",
    "AddCoreScriptLocal", "SaveScriptProfilingData", "GetUserSubscriptionDetailsInternalAsync",
    "GetUserSubscriptionStatusAsync", "PerformBulkPurchase", "PerformCancelSubscription",
    "PerformPurchaseV2", "PerformSubscriptionPurchase", "PerformSubscriptionPurchaseV2",
    "PrepareCollectiblesPurchase", "PromptBulkPurchase", "PromptCancelSubscription",
    "PromptCollectiblesPurchase", "PromptGamePassPurchase", "PromptNativePurchaseWithLocalPlayer",
    "PromptPremiumPurchase", "PromptRobloxPurchase", "PromptSubscriptionPurchase",
    "ReportAbuse", "ReportAbuseV3", "ReturnToJavaScript", "OpenNativeOverlay",
    "OpenWeChatAuthWindow", "EmitHybridEvent", "OpenUrl", "PostAsync", "PostAsyncFullUrl",
    "RequestLimitedAsync", "Load", "CaptureScreenshot", "CreatePostAsync", "DeleteCapture",
    "DeleteCapturesAsync", "GetCaptureFilePathAsync", "SaveCaptureToExternalStorage",
    "SaveCapturesToExternalStorageAsync", "GetCaptureUploadDataAsync", "RetrieveCaptures",
    "SaveScreenshotCapture", "Call", "GetProtocolMethodRequestMessageId",
    "GetProtocolMethodResponseMessageId", "PublishProtocolMethodRequest",
    "PublishProtocolMethodResponse", "Subscribe", "SubscribeToProtocolMethodRequest",
    "SubscribeToProtocolMethodResponse", "GetDeviceIntegrityToken", "GetDeviceIntegrityTokenYield",
    "NoPromptCreateOutfit", "NoPromptDeleteOutfit", "NoPromptRenameOutfit", "NoPromptSaveAvatar",
    "NoPromptSaveAvatarThumbnailCustomization", "NoPromptSetFavorite", "NoPromptUpdateOutfit",
    "PerformCreateOutfitWithDescription", "PerformRenameOutfit", "PerformSaveAvatarWithDescription",
    "PerformSetFavorite", "PerformUpdateOutfit", "PromptCreateOutfit", "PromptDeleteOutfit",
    "PromptRenameOutfit", "PromptSaveAvatar", "PromptSetFavorite", "PromptUpdateOutfit"
};

int IndexHook(lua_State* L)
{
    std::string Key = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";

    for (const char* Function : UnsafeFunction)
    {
        if (Key == Function)
        {
            Roblox::Print(3, "Function '%s' has been disabled for security reasons", Function);
            return 0;
        }
    }

    if (L->userdata && (L->userdata->Script.expired() || L->userdata->Capabilities == MaxCapabilities))
    {
        if (Key == "HttpGet" || Key == "HttpGetAsync")
        {
            lua_pushcclosure(L, Http::HttpGet, nullptr, 0);
            return 1;
        }
        else if (Key == "GetObjects")
        {
            lua_pushcclosure(L, Http::GetObjects, nullptr, 0);
            return 1;
        }
    }

    return OriginalIndex(L);
};

int NamecallHook(lua_State* L)
{
    std::string Key = L->namecall->data;

    for (const char* Function : UnsafeFunction)
    {
        if (Key == Function)
        {
            Roblox::Print(3, "Function '%s' has been disabled for security reasons", Function);
            return 0;
        }
    }

    if (L->userdata && (L->userdata->Script.expired() || L->userdata->Capabilities == MaxCapabilities))
    {
        if (Key == "HttpGet" || Key == "HttpGetAsync")
        {
            return Http::HttpGet(L);
        }
        else if (Key == "GetObjects")
        {
            return Http::GetObjects(L);
        }
    }

    return OriginalNamecall(L);
};

void cfg_add_page(int64_t r8_2)
{
    int64_t r9_13 = r8_2 & 0xfffffffffffff000;
    int32_t* r11_30 = (int32_t*)((r9_13 >> 0xf) + ((uintptr_t)GetModuleHandleA("RobloxPlayerBeta.dll") + 0x1682ab0));
    *r11_30 |= 1 << (((r8_2 & 0xfffff000) >> 0xc & 7) % 0x20);
}

void InitializeHooks(lua_State* L)
{
    int OriginalTop = lua_gettop(L);

    lua_getglobal(L, "game");
    luaL_getmetafield(L, -1, "__index");
    if (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTUSERDATA)
    {
        Closure* IndexClosure = clvalue(luaA_toobject(L, -1));
        OriginalIndex = IndexClosure->c.f;
        IndexClosure->c.f = IndexHook;
    }
    lua_pop(L, 1);

    luaL_getmetafield(L, -1, "__namecall");
    if (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTUSERDATA)
    {
        Closure* NamecallClosure = clvalue(luaA_toobject(L, -1));
        OriginalNamecall = NamecallClosure->c.f;
        NamecallClosure->c.f = NamecallHook;
    }
    lua_pop(L, 1);

    lua_settop(L, OriginalTop);
}

void Environment::SetupEnvironment(lua_State* L)
{
    luaL_sandboxthread(L);

    Cache::RegisterLibrary(L);
    Closures::RegisterLibrary(L);
    Http::RegisterLibrary(L);
    Miscellaneous::RegisterLibrary(L);
    Filesystem::RegisterLibrary(L);
    Script::RegisterLibrary(L);
    Debug::RegisterLibrary(L);
    Metatable::RegisterLibrary(L);
    Crypt::RegisterLibrary(L);
    Instance::RegisterLibrary(L);
    Interactions::RegisterLibrary(L);
    System::RegisterLibrary(L);
    Websocket::register_library(L);

    // todo: fix hookfunc (make pass on sunc), fix filtergc, fix checkcaller? fix getconnections ( returned false for a luaconnection[2] )

    InitializeHooks(L);
    luaL_sandboxthread(L);

	lua_newtable(L);
	lua_setglobal(L, "_G");

	lua_newtable(L);
	lua_setglobal(L, "shared");

    std::string environment_script = "loadstring(game:HttpGet('https://raw.githubusercontent.com/loopmetamethod/executor/refs/heads/main/env.luau'))()";
    std::string drawing_script = "loadstring(game:HttpGet('https://raw.githubusercontent.com/loopmetamethod/executor/refs/heads/main/drawing.luau'))()";

    TaskScheduler::RequestExecution(environment_script);
    TaskScheduler::RequestExecution(drawing_script);
}