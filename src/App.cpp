
//--------------------------
// Includes
//--------------------------
#include "App.h"
#include <cassert>


namespace {
	const auto ClassName = TEXT("SampleWindowClass");	//!< ウィンドウクラス名

	template<typename T>
	void SafeRelease(T*& ptr)
	{
		if (ptr != nullptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
	}
}

//--------------------------
//		コンストラクタ
//--------------------------

App::App(uint32_t width, uint32_t height)
	: m_hInst(nullptr)
	, m_hWnd(nullptr)
	, m_Width(width)
	, m_Height(height)
	, m_pDevice(nullptr)
	, m_pQueue(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCmdList(nullptr)
	, m_pHeapRTV(nullptr)
	, m_pFence(nullptr)
	, m_FrameIndex(0)
{
	for (auto i = 0u; i < FrameCount; ++i)
	{
		m_pColorBuffer[i] = nullptr;
		m_pCmdAllocator[i] = nullptr;
		m_FenceCounter[i] = 0;
	}
}


//--------------------------
//		デストラクタ
//--------------------------
App::~App()
{
	/* Do Nothing */
}



//--------------------------
//		実行
//--------------------------
void App::Run()
{
	if (InitApp())
	{
		MainLoop();
	}

	TermApp();
}


//--------------------------
//		初期化処理
//--------------------------
bool App::InitApp()
{
	// ウィンドウの初期化
	if (!InitWnd())
	{
		return false;
	}
	// DirectX3D12の初期化
	if (!InitD3D())
	{
		return false;
	}

	// 正常終了
	return true;
}


//-------------------------
//		終了処理
//-------------------------
void App::TermApp()
{
	// DirectX3D12の終了処理
	TermD3D();
	// ウィンドウの終了処理
	TermWnd();
}


//--------------------------
//	ウィンドウの初期化処理
//--------------------------
bool App::InitWnd()
{
	// インスタンスハンドルの取得
	auto hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// ウィンドウの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;	// ウィンドウスタイル
	wc.lpfnWndProc = WndProc;			// ウィンドウプロシージャ
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);	// アイコン
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);	// カーソル
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);	// 背景色
	wc.lpszMenuName = nullptr;			// メニュー名
	wc.lpszClassName = ClassName;			// クラス名
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);	// アイコン(小)

	// ウィンドウクラスの登録
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// インスタンスハンドルの設定
	m_hInst = hInst;

	// ウィンドウのサイズを設定
	RECT rc = {};
	rc.right = static_cast<LONG>(m_Width);
	rc.bottom = static_cast<LONG>(m_Height);

	// ウィンドウのサイズを補正
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// ウィンドウの生成
	m_hWnd = CreateWindowExW(
		0,
		ClassName,
		TEXT("Sample"),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInst,
		nullptr);

	if (m_hWnd == nullptr)
	{
		return false;
	}

	// ウィンドウの表示
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// ウィンドウの更新
	UpdateWindow(m_hWnd);

	// ウィンドウにフォーカスを設定
	SetFocus(m_hWnd);

	// 正常終了
	return true;

}


//--------------------------
//	ウィンドウの終了処理
//--------------------------
void App::TermWnd()
{
	// ウィンドウクラスの登録解除
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}



//--------------------------
// Direct3D12の初期化処理
//--------------------------
bool App::InitD3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	// デバッグレイヤーの有効化
	{
		ComPtr<ID3D12Debug> debug;
		auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));

		if (SUCCEEDED(hr))
		{
			debug->EnableDebugLayer();
		}
	}
#endif


	// デバイスの生成
	auto hr = D3D12CreateDevice(
		nullptr,	// デフォルトアダプタを使用
		D3D_FEATURE_LEVEL_11_0,	// DirectX 11.0
		IID_PPV_ARGS(m_pDevice.GetAddressOf()));	// デバイス

	if (FAILED(hr)) {
		return false;
	}

	// コマンドキューの生成
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// コマンドリストの種類
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// 優先度
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// フラグ
		desc.NodeMask = 0;	// ノードマスク

		hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.GetAddressOf()));
		if (FAILED(hr)) {
			return false;
		}
	}

	// スワップチェインの生成
	{
		// DXGIファクトリーの生成
		ComPtr<IDXGIFactory4> pFactory = nullptr;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
		if (FAILED(hr)) {
			return false;
		}

		// スワップチェインの設定
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = m_Width;	// バッファの幅
		desc.BufferDesc.Height = m_Height;	// バッファの高さ
		desc.BufferDesc.RefreshRate.Numerator = 60;	// リフレッシュレート(分子)
		desc.BufferDesc.RefreshRate.Denominator = 1;	// リフレッシュレート(分母)
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	// スキャンラインの並び順
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	// スケーリング
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// フォーマット
		desc.SampleDesc.Count = 1;	// マルチサンプリングの数
		desc.SampleDesc.Quality = 0;	// マルチサンプリングの品質
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// バッファの使用法
		desc.BufferCount = FrameCount;	// バッファの数
		desc.OutputWindow = m_hWnd;	// ウィンドウハンドル
		desc.Windowed = TRUE;	// ウィンドウモード
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// スワップエフェクト
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// フラグ

		// スワップチェインの生成
		ComPtr<IDXGISwapChain> pSwapChain = nullptr;
		hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, &pSwapChain);
		if (FAILED(hr)) {
			return false;
		}

		// IDXGISwapChain3の取得
		hr = pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.GetAddressOf()));
		if (FAILED(hr)) {
			pFactory.Reset();
			pSwapChain.Reset();
			return false;
		}

		// バックバッファ番号の取得
		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// 不要になったので解放
		pFactory.Reset();
		pSwapChain.Reset();
	}

	// コマンドアロケータの生成
	{
		for (auto i = 0u; i < FrameCount; ++i)
		{
			hr = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,	// コマンドリストの種類
				IID_PPV_ARGS(m_pCmdAllocator[i].GetAddressOf()));	// コマンドアロケータ

			if (FAILED(hr)) {
				return false;
			}
		}
	}

	// コマンドリストの生成
	{
		hr = m_pDevice->CreateCommandList(
			0,	// ノードマスク
			D3D12_COMMAND_LIST_TYPE_DIRECT,	// コマンドリストの種類
			m_pCmdAllocator[m_FrameIndex].Get(),	// コマンドアロケータ
			nullptr,	// パイプラインステートオブジェクト
			IID_PPV_ARGS(m_pCmdList.GetAddressOf()));	// コマンドリスト

		if (FAILED(hr)) {
			return false;
		}
	}

	// レンダーターゲットビューの生成
	{
		// ディスクリプタヒープの設定
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = FrameCount;	// ディスクリプタの数
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// ディスクリプタの種類
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// フラグ
		desc.NodeMask = 0;	// ノードマスク

		// ディスクリプタヒープの生成
		hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pHeapRTV.GetAddressOf()));
		if (FAILED(hr)) {
			return false;
		}

		auto handle = m_pHeapRTV->GetCPUDescriptorHandleForHeapStart();
		auto incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (auto i = 0u; i < FrameCount; i++)
		{
			// バッファの取得
			hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pColorBuffer[i].GetAddressOf()));
			if (FAILED(hr)) {
				return false;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	// フォーマット
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	// ビューの種類
			viewDesc.Texture2D.MipSlice = 0;	// ミップマップスライス
			viewDesc.Texture2D.PlaneSlice = 0;	// プレーンスライス

			// レンダーターゲットビューの生成
			m_pDevice->CreateRenderTargetView(m_pColorBuffer[i].Get(), &viewDesc, handle);

			// ハンドルの更新
			m_HandleRTV[i] = handle;
			handle.ptr += incrementSize;
		}
	}

	// フェンスの生成
	{
		// フェンスカウンターをリセット
		for (auto i = 0u; i < FrameCount; ++i)
		{
			m_FenceCounter[i] = 0;
		}

		// フェンスの生成
		hr = m_pDevice->CreateFence(
			m_FenceCounter[m_FrameIndex],	// 初期値
			D3D12_FENCE_FLAG_NONE,	// フラグ
			IID_PPV_ARGS(m_pFence.GetAddressOf()));	// フェンス

		if (FAILED(hr)) {
			return false;
		}

		m_FenceCounter[m_FrameIndex]++;

		// イベントの生成
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr) {
			return false;
		}
	}

	// コマンドリストのクローズ
	m_pCmdList->Close();

	// 正常終了
	return true;

}



//--------------------------
//	ウィンドウプロシージャ
//--------------------------
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		default:
		{
			/*Do Nothing*/
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wp, lp);
}


//--------------------------
//		メインループ
//--------------------------
void App::MainLoop()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			// メッセージの処理
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// 描画処理
			Render();
		}
	}
}


//--------------------------
//		描画処理
//--------------------------
void App::Render()
{
	// コマンドの記録を開始
	m_pCmdAllocator[m_FrameIndex]->Reset();
	m_pCmdList->Reset(m_pCmdAllocator[m_FrameIndex].Get(), nullptr);

	// リソースバリアの設定
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// バリアの種類
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// フラグ
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();	// リソース
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;	// バリア前の状態
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// バリア後の状態
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// サブリソース

	// リソースバリアの設定
	m_pCmdList->ResourceBarrier(1, &barrier);

	// レンダーターゲットの設定
	m_pCmdList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, nullptr);

	// クリアカラーの設定
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// レンダーターゲットビューのクリア
	m_pCmdList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

	// 描画処理
	{
		// ポリゴン描画用の処理を追加
	}

	// リソースバリアの設定
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// バリアの種類
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// フラグ
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();	// リソース
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;	// バリア前の状態
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;	// バリア後の状態
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// サブリソース

	// リソースバリアの設定
	m_pCmdList->ResourceBarrier(1, &barrier);

	// コマンドの記録を終了
	m_pCmdList->Close();

	// コマンドの実行
	ID3D12CommandList* ppCmdLists[] = { m_pCmdList.Get()};
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	// 画面に表示
	Present(1);

}


//--------------------------
// 画面表示、次のフレームの準備
//--------------------------
void App::Present(uint32_t interval)
{
	// 画面に表示
	m_pSwapChain->Present(interval, 0);

	// シグナル処理
	const auto currentValue = m_FenceCounter[m_FrameIndex];
	m_pQueue->Signal(m_pFence.Get(), currentValue);

	// バックバッファ番号の更新
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 次のフレームの描画準備が終わるまで待機
	if (m_pFence->GetCompletedValue() < m_FenceCounter[m_FrameIndex])
	{
		m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
		WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
	}

	// フェンスカウンターを更新
	m_FenceCounter[m_FrameIndex] = currentValue + 1;
}


//--------------------------
// GPUの処理完了を待機
//--------------------------
void App::WaitGpu()
{
	// nullptrチェック
	assert(m_pQueue != nullptr);
	assert(m_pFence != nullptr);
	assert(m_FenceEvent != nullptr);

	// シグナル処理
	m_pQueue->Signal(m_pFence.Get(), m_FenceCounter[m_FrameIndex]);

	// 完了時にイベントを設定
	m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

	// 待機処理
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

	// フェンスカウンターを更新
	m_FenceCounter[m_FrameIndex]++;
}


//--------------------------
// Direct3D12の終了処理
//--------------------------
void App::TermD3D()
{
	// GPUの処理完了を待機
	WaitGpu();

	// イベントの破棄
	if (m_FenceEvent != nullptr)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}

	// フェンスの破棄
	m_pFence.Reset();

	// レンダーターゲットビューの破棄
	m_pHeapRTV.Reset();
	for (auto i = 0u; i < FrameCount; ++i)
	{
		m_pColorBuffer[i].Reset();
	}

	// コマンドリストの破棄
	m_pCmdList.Reset();

	// コマンドアロケータの破棄
	for (auto i = 0u; i < FrameCount; ++i)
	{
		m_pCmdAllocator[i].Reset();
	}

	// スワップチェインの破棄
	m_pSwapChain.Reset();

	// コマンドキューの破棄
	m_pQueue.Reset();

	// デバイスの破棄
	m_pDevice.Reset();
}

