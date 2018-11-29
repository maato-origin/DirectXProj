#include "graphics.h"

#include <vector>
#include <memory>

#include "DirectXTex\WICTextureLoader\WICTextureLoader.h"
#include "DirectXTex\DDSTextureLoader\DDSTextureLoader.h"

using namespace DirectX;

namespace
{
	//頂点データ構造体
	struct VertexData
	{
		FLOAT x, y, z;
		FLOAT tx, ty;
	};

	//シェーダ定数バッファ
	struct ConstBuffer
	{
		XMMATRIX mtxProj;
		XMMATRIX mtxView;
		XMMATRIX mtxWorld;
		XMVECTOR diffuse;
	};
}

HRESULT Graphics::init(HWND hwnd)
{
	input.reset();

	pCamera->iniDist = 17.0f;
	pCamera->iniRotH = XM_PI;
	pCamera->iniRotV = XM_PI * 0.15f / 180.0f;
	pCamera->iniAt = XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f);
	pCamera->reset();

	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT cdev_flags = 0;

#ifndef  _DEBUG
	cdev_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // ! _DEBUG

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

	//デバイス作成
	HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, cdev_flags,
									&feature_level, 1, D3D11_SDK_VERSION, &pDevice,
									NULL, &pDeviceContext);

	//使用可能なMSAAを取得
	DXGI_SAMPLE_DESC sample_desc;
	int max_count = 8;	//D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT
	if (max_count > 8)
		max_count = 8;
	for (int i = 0; i <= max_count; i++)
	{
		UINT quality;
		if (SUCCEEDED(pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, i, &quality)))
		{
			if (0 < quality)
			{
				sample_desc.Count = i;
				sample_desc.Quality = quality - 1;
			}
		}
	}

	ComPtr<IDXGIDevice> dxgi_device;
	hr = pDevice->QueryInterface(IID_PPV_ARGS(&dxgi_device));
	if (FAILED(hr))
	{
		return hr;
	}
	ComPtr<IDXGIAdapter> dxgi_adapter;
	hr = dxgi_device->GetAdapter(&dxgi_adapter);
	if (FAILED(hr))
	{
		return hr;
	}
	ComPtr<IDXGIFactory> dxgi_factory;
	hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));
	if (FAILED(hr))
	{
		return hr;
	}

	//スワップチェイン設定
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc = sample_desc;
	swapChainDesc.Windowed = TRUE;

	//スワップチェイン作成
	hr = dxgi_factory->CreateSwapChain(pDevice.Get(), &swapChainDesc, &pSwapChain);
	if (FAILED(hr))
	{
		return hr;
	}

	hWnd = hwnd;

	//スワップチェインに用意されたバッファ(2Dテクスチャ)を取得
	ComPtr<ID3D11Texture2D> back_buffer;
	hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	if (FAILED(hr))
	{
		return hr;
	}

	//デプスバッファ作成
	D3D11_TEXTURE2D_DESC desc_depth;
	ZeroMemory(&desc_depth, sizeof(desc_depth));
	desc_depth.Width = width;
	desc_depth.Height = height;
	desc_depth.MipLevels = 1;
	desc_depth.ArraySize = 1;
	desc_depth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc_depth.SampleDesc = sample_desc;
	desc_depth.Usage = D3D11_USAGE_DEFAULT;
	desc_depth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc_depth.CPUAccessFlags = 0;
	desc_depth.MiscFlags = 0;
	hr = pDevice->CreateTexture2D(&desc_depth, NULL, &pDepthStencil);
	if (FAILED(hr))
	{
		return hr;
	}

	//DepthStencilView作成
	hr = pDevice->CreateDepthStencilView(pDepthStencil.Get(), nullptr, &pDepthStencilView);
	if (FAILED(hr))
	{
		return hr;
	}

	//レンダーターゲットビュー作成
	hr = pDevice->CreateRenderTargetView(back_buffer.Get(), NULL, &pRenderTargetView);
	if (FAILED(hr))
	{
		return hr;
	}

	//レンダーターゲットビューの登録
	ID3D11RenderTargetView* renderTargetView[1] = { pRenderTargetView.Get() };
	pDeviceContext->OMSetRenderTargets(1, renderTargetView, pDepthStencilView.Get());

	//ビューポート
	viewPort.Width = static_cast<FLOAT>(width);
	viewPort.Height = static_cast<FLOAT>(height);
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	pDeviceContext->RSSetViewports(1, &viewPort);

	//定数バッファ共通データ
	{
		//プロジェクション行列
		FLOAT aspect = viewPort.Width / viewPort.Height;	//アスペクト比
		FLOAT min_Z = 0.01f;
		FLOAT max_Z = 1000.0f;
		FLOAT fov = XM_PIDIV4;	//画角
		pCBCommon->mtxProj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(fov, aspect, min_Z, max_Z));

		//カメラ行列
		pCBCommon->mtxView = XMMatrixTranspose(pCamera->getViewMatrix());

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(CBuffCommon);
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA init;
		ZeroMemory(&init, sizeof(init));
		init.pSysMem = pCBCommon.get();

		hr = pDevice->CreateBuffer(&bufferDesc, &init, &pCBuffCommon);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	//地面ポリゴン作成
	hr = createGround(primitiveGround);
	if (FAILED(hr))
	{
		return hr;
	}

	//三角形ポリゴン作成
	hr = createTriangle(primitiveTriangle);
	if (FAILED(hr))
	{
		return hr;
	}

	auto& tri0 = triInfo->aBuff[0];
	tri0.diffuse = XMVectorSet(1, 1, 1, 1);
	tri0.mtxWorld = XMMatrixIdentity();

	//ラムダ式を使用
	srand(12345);
	auto randf = []() {return (float)rand() / (float)RAND_MAX; };
	auto randrf = [randf](float mn, float mx) {return randf()*(mx - mn) + mn; };

	//配置
	for (int i = 0; i < TRI_NUM; ++i)
	{
		auto& tri = triInfo->aBuff[i];
		tri.diffuse = XMVectorSet(randrf(0.5f, 1.0f), randrf(0.5f, 1.0f), randrf(0.5f, 1.0f), 1);
		XMVECTOR position = XMVectorSet(randrf(-50.0f, 50.0f), 0, randrf(5.0f, 80.0f), 1);
		tri.mtxWorld = XMMatrixTranspose(XMMatrixTranslationFromVector(position));
	}

	return hr;
}

void Graphics::update(const MouseCapture& mouse)
{
	//キー入力
	input.update(mouse);

	FLOAT step_time = 1.0f / 60.0f;
	pCamera->ctrl(input, step_time);
	
	rotateY += 0.01f;
	if (rotateY > XM_2PI)
	{
		rotateY -= XM_2PI;
	}
}

void Graphics::render()
{
	//カメラ行列更新
	pCBCommon->mtxView = XMMatrixTranspose(pCamera->getViewMatrix());
	pDeviceContext->UpdateSubresource(pCBuffCommon.Get(), 0, NULL, pCBCommon.get(), 0, 0);

	//指定色で画面クリア
	float clearColor[4] = { 0.3f,0.5f,0.8f,1.0f };
	pDeviceContext->ClearRenderTargetView(pRenderTargetView.Get(), clearColor);
	pDeviceContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	draw(primitiveGround);
	for (int i = 0; i < TRI_NUM; ++i)
	{
		auto& tri = triInfo->aBuff[i];
		pDeviceContext->UpdateSubresource(primitiveTriangle.apCBuffer[1].Get(), 0, NULL, &tri, 0, 0);
		draw(primitiveTriangle);
	}

	//結果をウィンドウに反映
	pSwapChain->Present(0, 0);
}

void Graphics::draw(DrawPrimitive& primitive)
{
	//頂点バッファ
	UINT vb_slot = 0;
	ID3D11Buffer* vb[1] = { primitive.pVertexBuffer.Get() };
	UINT stride[1] = { primitive.nStride };
	UINT offset[1] = { 0 };
	pDeviceContext->IASetVertexBuffers(vb_slot, 1, vb, stride, offset);

	//入力レイアウト
	pDeviceContext->IASetInputLayout(primitive.pVertexLayout.Get());

	//インデックスバッファ
	pDeviceContext->IASetIndexBuffer(primitive.pIndexBuffer.Get(), primitive.fmtIndex, 0);

	//プリミティブ形状(三角形リスト)
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//シェーダ
	pDeviceContext->VSSetShader(primitive.pVertexShader.Get(), nullptr, 0);
	pDeviceContext->PSSetShader(primitive.pPixelShader.Get(), nullptr, 0);

	//サンプラー
	UINT smp_slot = 0;
	ID3D11SamplerState* smp[1] = { primitive.pSampler.Get() };
	pDeviceContext->PSSetSamplers(smp_slot, 1, smp);

	//シェーダーリソースビュー(テクスチャ)
	UINT srv_slot = 0;
	ID3D11ShaderResourceView* srv[1] = { primitive.pShaderResourceView.Get() };
	pDeviceContext->PSSetShaderResources(srv_slot, 1, srv);

	//定数バッファ
	UINT cb_slot = 0;
	ID3D11Buffer* cb[2] = { primitive.apCBuffer[0].Get(),primitive.apCBuffer[1].Get() };
	pDeviceContext->VSSetConstantBuffers(cb_slot, 2, cb);
	pDeviceContext->PSSetConstantBuffers(cb_slot, 2, cb);

	//ラスタライザステート
	pDeviceContext->RSSetState(primitive.pRasterizerState.Get());

	//デプスステンシルステート
	pDeviceContext->OMSetDepthStencilState(primitive.pDepthStencilState.Get(), 0);

	//ブレンドステート
	pDeviceContext->OMSetBlendState(primitive.pBlendState.Get(), NULL, 0xffffffff);

	//ポリゴン描画
	pDeviceContext->DrawIndexed(primitive.numIndex, 0, 0);
}

//三角形ポリゴン
HRESULT Graphics::createTriangle(DrawPrimitive& primitive)
{
	//入力レイアウト定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT element_num = ARRAYSIZE(layout);

	//頂点バッファ作成
	VertexData vertice[3] = {
		{0.0f, 10.0f, 0.0f, 0.5f, 0.0f},
		{5.0f, 0.0f, 0.0f, 1.0f, 1.0f},
		{-5.0f, 0.0f, 0.0f, 0.0f, 1.0f},
	};

	//インデックスバッファ
	UINT indices[3] = { 0,1,2 };

	CD3D11_DEFAULT default_state;
	CD3D11_RASTERIZER_DESC rsdesc(default_state);
	rsdesc.CullMode = D3D11_CULL_NONE;

	CD3D11_BLEND_DESC bddesc(default_state);
	bddesc.AlphaToCoverageEnable = TRUE;

#if 0
	auto& bdrt0 = bddesc.RenderTarget[0];
	bdrt0.BlendEnable = TRUE;
	bdrt0.BlendOp = D3D11_BLEND_OP_ADD;
	bdrt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bdrt0.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bdrt0.DestBlendAlpha = D3D11_BLEND_ZERO;
	bdrt0.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bdrt0.SrcBlendAlpha = D3D11_BLEND_ONE;
#endif

	CD3D11_DEPTH_STENCIL_DESC dsdesc(default_state);
	dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	DrawPrimitive::CInfo cinfo = {
		L"data\\vshader.cso",
		L"data\\pshader.cso",
		L"data\\hole.dds",
		layout,element_num,
		vertice,sizeof(VertexData) * 3,sizeof(VertexData),
		indices,sizeof(int) * 3,DXGI_FORMAT_R32_UINT,3,
		nullptr,&rsdesc,&dsdesc,&bddesc,
		XMVectorSet(1.0f,1.0f,1.0f,1.0f),
		XMMatrixTranspose(XMMatrixRotationY(0.0f))
	};

	return createPrimitive(primitive, cinfo);
}

//地面作成
HRESULT Graphics::createGround(DrawPrimitive& primitive)
{
	const int N = 20;		//ポリゴン分割数
	const float WH = 10.0f;	//ポリゴンの大きさ
	
	const float OFS = WH * N / 2.0f;
	const int VNUM = N * N * 4;
	const int INUM = N * N * 2 * 3;
	std::vector<VertexData> vertex(VNUM);
	std::vector<UINT> index(INUM);

	int vidx = 0;
	int iidx = 0;

	for(int nz=0;nz<N;++nz)
	{
		for (int nx = 0; nx < N; nx++)
		{
			VertexData& v0 = vertex[vidx];
			VertexData& v1 = vertex[vidx + 1];
			VertexData& v2 = vertex[vidx + 2];
			VertexData& v3 = vertex[vidx + 3];
			v0.x = nx * WH - OFS;
			v0.y = 0.0f;
			v0.z = nz * WH - OFS;
			v0.tx = 0.0f;
			v0.ty = 0.0f;

			v3 = v2 = v1 = v0;

			v1.x += WH;
			v1.tx = 1.0f;

			v2.z += WH;
			v2.ty = 1.0f;

			v3.x += WH;
			v3.z += WH;
			v3.tx = v3.ty = 1.0f;

			index[iidx++] = vidx + 0;
			index[iidx++] = vidx + 2;
			index[iidx++] = vidx + 1;
			index[iidx++] = vidx + 1;
			index[iidx++] = vidx + 2;
			index[iidx++] = vidx + 3;
			vidx += 4;
		}
	}

	//入力レイアウト定義
	D3D11_INPUT_ELEMENT_DESC layout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },	
	};
	UINT element_num = ARRAYSIZE(layout);

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.MaxAnisotropy = 8;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	CD3D11_DEFAULT default_state;
	CD3D11_DEPTH_STENCIL_DESC dsdesc(default_state);
	dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	DrawPrimitive::CInfo cinfo = 
	{
		L"data\\vshader.cso",
		L"data\\pshader.cso",
		L"data\\check.dds",
		layout,element_num,
		vertex.data(),sizeof(VertexData)*VNUM,sizeof(VertexData),
		index.data(),sizeof(int)*INUM,DXGI_FORMAT_R32_UINT,INUM,
		&samplerDesc,nullptr,&dsdesc,nullptr,
		XMVectorSet(0.95f,0.75f,0.5f,1.0f),
		XMMatrixTranspose(XMMatrixRotationY(0.0f))
	};

	return createPrimitive(primitive, cinfo);
}

HRESULT Graphics::createPrimitive(DrawPrimitive& primitive, const DrawPrimitive::CInfo& cinfo)
{
	DrawPrimitive drawPrimitive;

	//シェーダ読み込み
	FileRead vscode(cinfo.vs_path);
	FileRead pscode(cinfo.ps_path);

	HRESULT hr = pDevice->CreateVertexShader(vscode.get(), vscode.size(), NULL, &drawPrimitive.pVertexShader);
	if (FAILED(hr))
	{
		return hr;
	}

	//ピクセルシェーダ作成
	hr = pDevice->CreatePixelShader(pscode.get(), pscode.size(), NULL, &drawPrimitive.pPixelShader);
	if (FAILED(hr))
	{
		return hr;
	}

	//入力レイアウト定義
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT element_num = ARRAYSIZE(layout);

	//入力レイアウト作成
	hr = pDevice->CreateInputLayout(cinfo.layout, cinfo.layout_num, vscode.get(), vscode.size(), &drawPrimitive.pVertexLayout);
	if (FAILED(hr))
	{
		return hr;
	}

	//頂点バッファ
	{
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = cinfo.vertex_size;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(initData));
		initData.pSysMem = cinfo.vertex;
		hr = pDevice->CreateBuffer(&bufferDesc, &initData, &drawPrimitive.pVertexBuffer);
		if (FAILED(hr))
		{
			return hr;
		}
		drawPrimitive.nStride = sizeof(VertexData);
	}

	//インデックスバッファ
	{
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = cinfo.index_size;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(initData));
		initData.pSysMem = cinfo.index;
		hr = pDevice->CreateBuffer(&bufferDesc, &initData, &drawPrimitive.pIndexBuffer);
		if (FAILED(hr))
		{
			return hr;
		}
		drawPrimitive.numIndex = cinfo.index_num;
	}

	//テクスチャ作成
	std::wstring tpath = cinfo.texture_path;
	std::transform(tpath.begin(), tpath.end(), tpath.begin(), tolower);

	if (std::wstring::npos != tpath.rfind(L".dds"))
	{
		hr = DirectX::CreateDDSTextureFromFile(pDevice.Get(), cinfo.texture_path, &drawPrimitive.pTexture, &drawPrimitive.pShaderResourceView);
	}
	else
	{
		hr = DirectX::CreateWICTextureFromFile(pDevice.Get(), cinfo.texture_path, &drawPrimitive.pTexture, &drawPrimitive.pShaderResourceView);
	}
	if (FAILED(hr))
	{
		return hr;
	}

	CD3D11_DEFAULT default_state;

	//サンプラー作成
	const D3D11_SAMPLER_DESC* samplerDesc = cinfo.sampler;
	D3D11_SAMPLER_DESC sampDesc = CD3D11_SAMPLER_DESC(default_state);
	if (!samplerDesc)
		samplerDesc = &sampDesc;
	hr = pDevice->CreateSamplerState(samplerDesc, &drawPrimitive.pSampler);
	if (FAILED(hr))
	{
		return hr;
	}

	//共通定数バッファ
	drawPrimitive.apCBuffer[0] = pCBuffCommon;

	//定数バッファ
	{
		CBuffMesh cBuff;
		cBuff.mtxWorld = cinfo.world_matrix;
		cBuff.diffuse = cinfo.diffuse;

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(ConstBuffer);
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA init;
		ZeroMemory(&init, sizeof(init));
		init.pSysMem = &cBuff;

		hr = pDevice->CreateBuffer(&bufferDesc, &init, &drawPrimitive.apCBuffer[1]);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	//ラスタライザステート
	const D3D11_RASTERIZER_DESC* rasterizerDesc = cinfo.rasterizer;
	CD3D11_RASTERIZER_DESC rsdesc(default_state);
	if (!rasterizerDesc)
		rasterizerDesc = &rsdesc;
	hr = pDevice->CreateRasterizerState(rasterizerDesc, &drawPrimitive.pRasterizerState);
	if (FAILED(hr))
	{
		return hr;
	}

	//デプスステンシルステート
	const D3D11_DEPTH_STENCIL_DESC* depthStencilDesc = cinfo.depthstencil;
	CD3D11_DEPTH_STENCIL_DESC dsdesc(default_state);
	if (!depthStencilDesc)
		depthStencilDesc = &dsdesc;
	hr = pDevice->CreateDepthStencilState(depthStencilDesc, &drawPrimitive.pDepthStencilState);
	if (FAILED(hr))
	{
		return hr;
	}

	//ブレンドステート
	const D3D11_BLEND_DESC* blendDesc = cinfo.blend;
	CD3D11_BLEND_DESC bddesc(default_state);
	if (!blendDesc)
		blendDesc = &bddesc;
	hr = pDevice->CreateBlendState(blendDesc, &drawPrimitive.pBlendState);
	if (FAILED(hr))
	{
		return hr;
	}

	primitive = std::move(drawPrimitive);
	return S_OK;
}