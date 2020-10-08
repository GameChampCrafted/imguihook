#include "includes.h"
#include "imgui_memory_editor.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDeviceD3D11 = NULL;
ID3D11DeviceContext* pContextD3D11 = NULL;
ID3D11RenderTargetView* mainRenderTargetViewD3D11;


void InitImGuiD3D11()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	
	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDeviceD3D11, pContextD3D11);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return true;
	}
	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}
bool show = false;
bool init = false;

HRESULT __stdcall hkPresent_D3D11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDeviceD3D11)))
		{
			pDeviceD3D11->GetImmediateContext(&pContextD3D11);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDeviceD3D11->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGuiD3D11();
			init = true;
		}
		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	if (GetAsyncKeyState(VK_INSERT) & 1) {
		show = !show;
	}

	if (GetAsyncKeyState(VK_END)) {
		kiero::shutdown();
		return 0;
	}

	if (show) {
		
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		//Memory Editor Window
		static char data[] = { 0xE2,0x82,0xAC, 0 };
		size_t data_size = 0x10000;
		static MemoryEditor mem_edit;
		mem_edit.DrawWindow("Memory Editor", data, data_size);

		ImGui::ShowDemoWindow();

		ImGui::Begin("ImGui Window");
		ImGui::Text("This is some useful text.");
		ImGui::End();

		ImGui::Render();
		pContextD3D11->OMSetRenderTargets(1, &mainRenderTargetViewD3D11, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	}
	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)& oPresent, hkPresent_D3D11);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}