#pragma once

//--------------------------
// Includes
//--------------------------
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>



//--------------------------
//		Linker
//--------------------------
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


//--------------------------
// Tyoe definitions
//--------------------------
template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;	// タイプ定義




//////////////////////////////
// Class name: App
//////////////////////////////
class App
{
	//==================================================
	// List of friend classes and methods
	//==================================================
	/* Nothing */

public:
	//==================================================
	// Public typedefs
	//==================================================
	/* Nothing */


	//==================================================
	// Public methods
	//==================================================
	App(uint32_t width, uint32_t height);
	~App();
	void Run();


private:
	//==================================================
	// private typedefs
	//==================================================
	static const uint32_t FrameCount = 2;	// フレームバッファの数

	HINSTANCE m_hInst;	// インスタンスハンドル
	HWND m_hWnd;		// ウィンドウハンドル
	uint32_t m_Width;	// ウィンドウの幅
	uint32_t m_Height;	// ウィンドウの高さ

	ComPtr<ID3D12Device> m_pDevice;	// デバイス
	ComPtr<ID3D12CommandQueue> m_pQueue;	// コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain;	// スワップチェイン
	ComPtr<ID3D12Resource> m_pColorBuffer[FrameCount];	// カラーバッファ
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator[FrameCount];	// コマンドアロケータ
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList;	// コマンドリスト
	ComPtr<ID3D12DescriptorHeap> m_pHeapRTV;	// レンダーターゲットビュー用ディスクリプタヒープ
	ComPtr<ID3D12Fence> m_pFence;	// フェンス
	HANDLE m_FenceEvent;	// フェンスのイベントハンドル
	uint64_t m_FenceCounter[FrameCount];	// フェンスのカウンタ値
	uint32_t m_FrameIndex;	// フレーム番号
	D3D12_CPU_DESCRIPTOR_HANDLE m_HandleRTV[FrameCount];	// レンダーターゲットビューのCPUディスクリプタハンドル



	//==================================================
	// Private methods
	//==================================================
	bool InitApp();
	void TermApp();
	bool InitWnd();
	void TermWnd();
	void MainLoop();
	bool InitD3D();
	void TermD3D();
	void Render();
	void WaitGpu();
	void Present(uint32_t interval);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};