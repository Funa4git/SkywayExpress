
//--------------------------
// Includes
//--------------------------
#include "App.h"
#include <cassert>


namespace {
	const auto ClassName = TEXT("SampleWindowClass");	//!< �E�B���h�E�N���X��

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
//		�R���X�g���N�^
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
//		�f�X�g���N�^
//--------------------------
App::~App()
{
	/* Do Nothing */
}



//--------------------------
//		���s
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
//		����������
//--------------------------
bool App::InitApp()
{
	// �E�B���h�E�̏�����
	if (!InitWnd())
	{
		return false;
	}
	// DirectX3D12�̏�����
	if (!InitD3D())
	{
		return false;
	}

	// ����I��
	return true;
}


//-------------------------
//		�I������
//-------------------------
void App::TermApp()
{
	// DirectX3D12�̏I������
	TermD3D();
	// �E�B���h�E�̏I������
	TermWnd();
}


//--------------------------
//	�E�B���h�E�̏���������
//--------------------------
bool App::InitWnd()
{
	// �C���X�^���X�n���h���̎擾
	auto hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// �E�B���h�E�̐ݒ�
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;	// �E�B���h�E�X�^�C��
	wc.lpfnWndProc = WndProc;			// �E�B���h�E�v���V�[�W��
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);	// �A�C�R��
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);	// �J�[�\��
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);	// �w�i�F
	wc.lpszMenuName = nullptr;			// ���j���[��
	wc.lpszClassName = ClassName;			// �N���X��
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);	// �A�C�R��(��)

	// �E�B���h�E�N���X�̓o�^
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// �C���X�^���X�n���h���̐ݒ�
	m_hInst = hInst;

	// �E�B���h�E�̃T�C�Y��ݒ�
	RECT rc = {};
	rc.right = static_cast<LONG>(m_Width);
	rc.bottom = static_cast<LONG>(m_Height);

	// �E�B���h�E�̃T�C�Y��␳
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// �E�B���h�E�̐���
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

	// �E�B���h�E�̕\��
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// �E�B���h�E�̍X�V
	UpdateWindow(m_hWnd);

	// �E�B���h�E�Ƀt�H�[�J�X��ݒ�
	SetFocus(m_hWnd);

	// ����I��
	return true;

}


//--------------------------
//	�E�B���h�E�̏I������
//--------------------------
void App::TermWnd()
{
	// �E�B���h�E�N���X�̓o�^����
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}



//--------------------------
// Direct3D12�̏���������
//--------------------------
bool App::InitD3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	// �f�o�b�O���C���[�̗L����
	{
		ComPtr<ID3D12Debug> debug;
		auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));

		if (SUCCEEDED(hr))
		{
			debug->EnableDebugLayer();
		}
	}
#endif


	// �f�o�C�X�̐���
	auto hr = D3D12CreateDevice(
		nullptr,	// �f�t�H���g�A�_�v�^���g�p
		D3D_FEATURE_LEVEL_11_0,	// DirectX 11.0
		IID_PPV_ARGS(m_pDevice.GetAddressOf()));	// �f�o�C�X

	if (FAILED(hr)) {
		return false;
	}

	// �R�}���h�L���[�̐���
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// �R�}���h���X�g�̎��
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// �D��x
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// �t���O
		desc.NodeMask = 0;	// �m�[�h�}�X�N

		hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.GetAddressOf()));
		if (FAILED(hr)) {
			return false;
		}
	}

	// �X���b�v�`�F�C���̐���
	{
		// DXGI�t�@�N�g���[�̐���
		ComPtr<IDXGIFactory4> pFactory = nullptr;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
		if (FAILED(hr)) {
			return false;
		}

		// �X���b�v�`�F�C���̐ݒ�
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = m_Width;	// �o�b�t�@�̕�
		desc.BufferDesc.Height = m_Height;	// �o�b�t�@�̍���
		desc.BufferDesc.RefreshRate.Numerator = 60;	// ���t���b�V�����[�g(���q)
		desc.BufferDesc.RefreshRate.Denominator = 1;	// ���t���b�V�����[�g(����)
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	// �X�L�������C���̕��я�
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	// �X�P�[�����O
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// �t�H�[�}�b�g
		desc.SampleDesc.Count = 1;	// �}���`�T���v�����O�̐�
		desc.SampleDesc.Quality = 0;	// �}���`�T���v�����O�̕i��
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// �o�b�t�@�̎g�p�@
		desc.BufferCount = FrameCount;	// �o�b�t�@�̐�
		desc.OutputWindow = m_hWnd;	// �E�B���h�E�n���h��
		desc.Windowed = TRUE;	// �E�B���h�E���[�h
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// �X���b�v�G�t�F�N�g
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// �t���O

		// �X���b�v�`�F�C���̐���
		ComPtr<IDXGISwapChain> pSwapChain = nullptr;
		hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, &pSwapChain);
		if (FAILED(hr)) {
			return false;
		}

		// IDXGISwapChain3�̎擾
		hr = pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.GetAddressOf()));
		if (FAILED(hr)) {
			pFactory.Reset();
			pSwapChain.Reset();
			return false;
		}

		// �o�b�N�o�b�t�@�ԍ��̎擾
		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// �s�v�ɂȂ����̂ŉ��
		pFactory.Reset();
		pSwapChain.Reset();
	}

	// �R�}���h�A���P�[�^�̐���
	{
		for (auto i = 0u; i < FrameCount; ++i)
		{
			hr = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,	// �R�}���h���X�g�̎��
				IID_PPV_ARGS(m_pCmdAllocator[i].GetAddressOf()));	// �R�}���h�A���P�[�^

			if (FAILED(hr)) {
				return false;
			}
		}
	}

	// �R�}���h���X�g�̐���
	{
		hr = m_pDevice->CreateCommandList(
			0,	// �m�[�h�}�X�N
			D3D12_COMMAND_LIST_TYPE_DIRECT,	// �R�}���h���X�g�̎��
			m_pCmdAllocator[m_FrameIndex].Get(),	// �R�}���h�A���P�[�^
			nullptr,	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g
			IID_PPV_ARGS(m_pCmdList.GetAddressOf()));	// �R�}���h���X�g

		if (FAILED(hr)) {
			return false;
		}
	}

	// �����_�[�^�[�Q�b�g�r���[�̐���
	{
		// �f�B�X�N���v�^�q�[�v�̐ݒ�
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = FrameCount;	// �f�B�X�N���v�^�̐�
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// �f�B�X�N���v�^�̎��
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �t���O
		desc.NodeMask = 0;	// �m�[�h�}�X�N

		// �f�B�X�N���v�^�q�[�v�̐���
		hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pHeapRTV.GetAddressOf()));
		if (FAILED(hr)) {
			return false;
		}

		auto handle = m_pHeapRTV->GetCPUDescriptorHandleForHeapStart();
		auto incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (auto i = 0u; i < FrameCount; i++)
		{
			// �o�b�t�@�̎擾
			hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pColorBuffer[i].GetAddressOf()));
			if (FAILED(hr)) {
				return false;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	// �t�H�[�}�b�g
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	// �r���[�̎��
			viewDesc.Texture2D.MipSlice = 0;	// �~�b�v�}�b�v�X���C�X
			viewDesc.Texture2D.PlaneSlice = 0;	// �v���[���X���C�X

			// �����_�[�^�[�Q�b�g�r���[�̐���
			m_pDevice->CreateRenderTargetView(m_pColorBuffer[i].Get(), &viewDesc, handle);

			// �n���h���̍X�V
			m_HandleRTV[i] = handle;
			handle.ptr += incrementSize;
		}
	}

	// �t�F���X�̐���
	{
		// �t�F���X�J�E���^�[�����Z�b�g
		for (auto i = 0u; i < FrameCount; ++i)
		{
			m_FenceCounter[i] = 0;
		}

		// �t�F���X�̐���
		hr = m_pDevice->CreateFence(
			m_FenceCounter[m_FrameIndex],	// �����l
			D3D12_FENCE_FLAG_NONE,	// �t���O
			IID_PPV_ARGS(m_pFence.GetAddressOf()));	// �t�F���X

		if (FAILED(hr)) {
			return false;
		}

		m_FenceCounter[m_FrameIndex]++;

		// �C�x���g�̐���
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr) {
			return false;
		}
	}

	// �R�}���h���X�g�̃N���[�Y
	m_pCmdList->Close();

	// ����I��
	return true;

}



//--------------------------
//	�E�B���h�E�v���V�[�W��
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
//		���C�����[�v
//--------------------------
void App::MainLoop()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			// ���b�Z�[�W�̏���
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// �`�揈��
			Render();
		}
	}
}


//--------------------------
//		�`�揈��
//--------------------------
void App::Render()
{
	// �R�}���h�̋L�^���J�n
	m_pCmdAllocator[m_FrameIndex]->Reset();
	m_pCmdList->Reset(m_pCmdAllocator[m_FrameIndex].Get(), nullptr);

	// ���\�[�X�o���A�̐ݒ�
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// �o���A�̎��
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// �t���O
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();	// ���\�[�X
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;	// �o���A�O�̏��
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// �o���A��̏��
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// �T�u���\�[�X

	// ���\�[�X�o���A�̐ݒ�
	m_pCmdList->ResourceBarrier(1, &barrier);

	// �����_�[�^�[�Q�b�g�̐ݒ�
	m_pCmdList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, nullptr);

	// �N���A�J���[�̐ݒ�
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// �����_�[�^�[�Q�b�g�r���[�̃N���A
	m_pCmdList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

	// �`�揈��
	{
		// �|���S���`��p�̏�����ǉ�
	}

	// ���\�[�X�o���A�̐ݒ�
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// �o���A�̎��
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// �t���O
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();	// ���\�[�X
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;	// �o���A�O�̏��
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;	// �o���A��̏��
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// �T�u���\�[�X

	// ���\�[�X�o���A�̐ݒ�
	m_pCmdList->ResourceBarrier(1, &barrier);

	// �R�}���h�̋L�^���I��
	m_pCmdList->Close();

	// �R�}���h�̎��s
	ID3D12CommandList* ppCmdLists[] = { m_pCmdList.Get()};
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	// ��ʂɕ\��
	Present(1);

}


//--------------------------
// ��ʕ\���A���̃t���[���̏���
//--------------------------
void App::Present(uint32_t interval)
{
	// ��ʂɕ\��
	m_pSwapChain->Present(interval, 0);

	// �V�O�i������
	const auto currentValue = m_FenceCounter[m_FrameIndex];
	m_pQueue->Signal(m_pFence.Get(), currentValue);

	// �o�b�N�o�b�t�@�ԍ��̍X�V
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// ���̃t���[���̕`�揀�����I���܂őҋ@
	if (m_pFence->GetCompletedValue() < m_FenceCounter[m_FrameIndex])
	{
		m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
		WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
	}

	// �t�F���X�J�E���^�[���X�V
	m_FenceCounter[m_FrameIndex] = currentValue + 1;
}


//--------------------------
// GPU�̏���������ҋ@
//--------------------------
void App::WaitGpu()
{
	// nullptr�`�F�b�N
	assert(m_pQueue != nullptr);
	assert(m_pFence != nullptr);
	assert(m_FenceEvent != nullptr);

	// �V�O�i������
	m_pQueue->Signal(m_pFence.Get(), m_FenceCounter[m_FrameIndex]);

	// �������ɃC�x���g��ݒ�
	m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

	// �ҋ@����
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

	// �t�F���X�J�E���^�[���X�V
	m_FenceCounter[m_FrameIndex]++;
}


//--------------------------
// Direct3D12�̏I������
//--------------------------
void App::TermD3D()
{
	// GPU�̏���������ҋ@
	WaitGpu();

	// �C�x���g�̔j��
	if (m_FenceEvent != nullptr)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}

	// �t�F���X�̔j��
	m_pFence.Reset();

	// �����_�[�^�[�Q�b�g�r���[�̔j��
	m_pHeapRTV.Reset();
	for (auto i = 0u; i < FrameCount; ++i)
	{
		m_pColorBuffer[i].Reset();
	}

	// �R�}���h���X�g�̔j��
	m_pCmdList.Reset();

	// �R�}���h�A���P�[�^�̔j��
	for (auto i = 0u; i < FrameCount; ++i)
	{
		m_pCmdAllocator[i].Reset();
	}

	// �X���b�v�`�F�C���̔j��
	m_pSwapChain.Reset();

	// �R�}���h�L���[�̔j��
	m_pQueue.Reset();

	// �f�o�C�X�̔j��
	m_pDevice.Reset();
}

