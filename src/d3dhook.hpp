#include "hook.hpp"

#include <d3d.h>
#include <d3d11.h>

#pragma once

typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* This, UINT SyncInterval, UINT Flags);

typedef VTableHook< D3D11PresentHook> PresentVTableHook;