#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>

#include <array>

#include "utility.h"
#include "controller.h"

using Microsoft::WRL::ComPtr;
using DirectX::XMMATRIX;
using DirectX::XMVECTOR;

//共通定数バッファ　透視行列など
struct CBuffCommon	//register(b0)
{
	XMMATRIX mtxProj;
	XMMATRIX mtxView;
};

//ポリゴンメッシュ用定数バッファ
struct CBuffMesh	//register(b1)
{
	XMMATRIX mtxWorld;
	XMVECTOR diffuse;
};

//ポリゴン描画
struct DrawPrimitive
{
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	ComPtr<ID3D11InputLayout> pVertexLayout;
	ComPtr<ID3D11Buffer> pVertexBuffer;
	UINT nStride = 0;
	ComPtr<ID3D11Buffer> pIndexBuffer;
	DXGI_FORMAT fmtIndex = DXGI_FORMAT_R32_UINT;
	UINT numIndex = 0;

	ComPtr<ID3D11Resource> pTexture;
	ComPtr<ID3D11ShaderResourceView> pShaderResourceView;
	ComPtr<ID3D11SamplerState> pSampler;
	std::array<ComPtr<ID3D11Buffer>, 2> apCBuffer;

	ComPtr<ID3D11RasterizerState> pRasterizerState;
	ComPtr<ID3D11DepthStencilState> pDepthStencilState;
	ComPtr<ID3D11BlendState> pBlendState;

	struct CInfo
	{
		const wchar_t* vs_path = nullptr;
		const wchar_t* ps_path = nullptr;

		const wchar_t* texture_path = nullptr;

		const D3D11_INPUT_ELEMENT_DESC* layout = nullptr;
		UINT layout_num = 0;

		const void* vertex = nullptr;
		UINT vertex_size = 0;
		UINT vertex_stride = 0;

		const void* index = nullptr;
		UINT index_size = 0;
		DXGI_FORMAT index_format = DXGI_FORMAT_R32_UINT;
		UINT index_num = 0;

		const D3D11_SAMPLER_DESC* sampler = nullptr;
		const D3D11_RASTERIZER_DESC* rasterizer = nullptr;
		const D3D11_DEPTH_STENCIL_DESC* depthstencil = nullptr;
		const D3D11_BLEND_DESC* blend = nullptr;

		XMVECTOR diffuse;
		XMMATRIX world_matrix;
	};
};

class Graphics
{
public:
	Graphics() {}
	~Graphics(){}

	HRESULT init(HWND hwnd);
	void update(const MouseCapture& mouse);
	void render();

private:
	HWND hWnd = NULL;
	KeyInput input;
	AlignedObject<CameraCtrl> pCamera;

	ComPtr<ID3D11Device> pDevice;
	ComPtr<ID3D11DeviceContext> pDeviceContext;
	ComPtr<IDXGISwapChain> pSwapChain;
	ComPtr<ID3D11Texture2D> pDepthStencil;
	ComPtr<ID3D11RenderTargetView> pRenderTargetView;
	ComPtr<ID3D11DepthStencilView> pDepthStencilView;
	D3D11_VIEWPORT viewPort;

	//DirectXMathのベクトルは16byte境界に配置
	AlignedObject<CBuffCommon> pCBCommon;	//共通定数バッファ
	ComPtr<ID3D11Buffer> pCBuffCommon;

	//ポリゴン回転角度
	FLOAT rotateY = 0.0f;

	//地面
	AlignedObject<CBuffMesh> pCBGround;
	DrawPrimitive primitiveGround;

	//三角形
	DrawPrimitive primitiveTriangle;
	static const int TRI_NUM = 50;
	struct TriCBuffs
	{
		std::array<CBuffMesh, TRI_NUM> aBuff;
	};
	AlignedObject<TriCBuffs> triInfo;

	void draw(DrawPrimitive& primitive);

	//地面作成
	HRESULT createGround(DrawPrimitive& primitive);

	//三角形ポリゴン
	HRESULT createTriangle(DrawPrimitive& primitive);

	HRESULT createPrimitive(DrawPrimitive& primitive, const DrawPrimitive::CInfo& cInfo);
};