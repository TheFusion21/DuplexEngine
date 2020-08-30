
// EXTERNAL INCLUDES

#include <iostream>
#define NOMINMAX
#include <d3dcompiler.h>
// INTERNAL INCLUDES
#include "d3d11renderer.h"
#include "utils/util.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "utils/color.h"
#include "vertex.h"
#include "transform.h"
//#include <spirv_cross/spirv_hlsl.hpp>
using namespace Engine::Math;
using namespace Engine::Utils;
using namespace Engine::Components;
using namespace Engine::Graphics;


bool D3D11Renderer::Init(ui64 instance, ui64 handle, ui32 width, ui32 height)
{
	this->width = width;
	this->height = height;
	IDXGIFactory1 * factory = nullptr;
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))))
	{
		MessageBoxA(NULL, "Could not create DX factory", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	//Adapter -> Interface for GPU
	IDXGIAdapter1* adapter = nullptr;
	if (FAILED(factory->EnumAdapters1(0, &adapter)))
	{
		MessageBoxA(NULL, "Could not enumerate adapter", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	//Output -> Interface for Displays
	IDXGIOutput* output = nullptr;
	if (FAILED(adapter->EnumOutputs(0, &output)))
	{
		MessageBoxA(NULL, "Could not enumerate outputs", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	//Retrieve the amount of modes available to us with this output
	ui32 numModes;
	if (FAILED(output->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr)))
	{
		MessageBoxA(NULL, "Could not retrieve display mode count", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	//Allocate array for the modes
	DXGI_MODE_DESC* modes = new DXGI_MODE_DESC[numModes];
	if (modes == nullptr)
	{
		MessageBoxA(NULL, "Could not allocate mode descriptions", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	//read all modes from the output
	if (FAILED(output->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, modes)))
	{
		MessageBoxA(NULL, "Could not retrieve display modes", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	ui32 refreshNum = 0;
	ui32 refreshDenom = 0;
	//Get the mode that matches our client size don't break so we get the one with highest refresh rate
	for (ui32 i = 0; i < numModes; i++)
	{
		if (modes[i].Width == width && modes[i].Height == height)
		{
			refreshNum = modes[i].RefreshRate.Numerator;
			refreshDenom = modes[i].RefreshRate.Denominator;
		}
	}
	
	//Let us retrieve the description for our Graphics card
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	if (FAILED(adapter->GetDesc1(&adapterDesc)))
	{
		MessageBoxA(NULL, "Could not retrieve adapter description", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	//Output the name and available memory to the console
	{
		char adapterDescText[128];
		wcstombs(adapterDescText, adapterDesc.Description, 128);
		printf("Graphics Device: %s\n", adapterDescText);
		printf("Graphics available Memory: %d MB\n", static_cast<ui32>(adapterDesc.DedicatedVideoMemory * 9.5367E-7f));
	}
	factory->MakeWindowAssociation(reinterpret_cast<HWND>(handle), DXGI_MWA_NO_ALT_ENTER);
	//Release and delete stuff we don't need anymore in next code section
	SAFEDELETEARR(modes);

	SAFERELEASE(output);
	SAFERELEASE(adapter);
	SAFERELEASE(factory);

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	//Double buffer
	swapChainDesc.BufferCount = 2;
	//Set Buffersize to client application size
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	//Set format to requested mode format --- needs to be the same as in GetDisplayModeList above
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	//Set Refreshrate as Rational if Vsync is enabled
	swapChainDesc.BufferDesc.RefreshRate.Numerator = (vsyncEnable ? refreshNum : 0);
	swapChainDesc.BufferDesc.RefreshRate.Denominator = (vsyncEnable && refreshDenom > 0 ? refreshDenom : 1);
	//We dont care how scanline is ordered or buffer is scaled
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	//mark this buffer for output
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//assign our created window to it
	swapChainDesc.OutputWindow = reinterpret_cast<HWND>(handle);
	// make lowest sampler
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	//our window was created windows
	swapChainDesc.Windowed = true;
	//Discard the when swap
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//No flags
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	
	//Feature level to request from D3D
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	if (FAILED(D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &context)))
	{
		MessageBoxA(NULL, "Could not create device and swap chain", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	if(!CreateRenderTarget())
	{
		return false;
	}


	if (!CreateDepthStencil())
	{
		return false;
	}

	//Set our rendertarget to the target view
	context->OMSetRenderTargets(1, &rtv, depthView);

	
	
	/*
	Create a rasterizer to tell the cpu what to cull and how the index winding is done
	Addionally a variable is addded to enable a wireframe mode just for fun
	*/
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FrontCounterClockwise = true;
	rasterDesc.CullMode = wireframe ? D3D11_CULL_NONE : D3D11_CULL_BACK;
	rasterDesc.MultisampleEnable = wireframe ? false : true;
	rasterDesc.AntialiasedLineEnable = wireframe;
	rasterDesc.FillMode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthBias = 0;
	if (FAILED(device->CreateRasterizerState(&rasterDesc, &rasterState)))
	{
		MessageBoxA(NULL, "Could not create raster State", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	context->RSSetState(rasterState);
	//Set our viewport to the size of our client size
	SetViewPort();
	modelConstant modelCB;

	modelBuffer = CreateBuffer(BufferType::Constant, &modelCB, sizeof(modelConstant), UsageType::Dynamic);
	if (!modelBuffer)
	{
		MessageBoxA(NULL, "Could not create transform buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	D3D11_SAMPLER_DESC defaultSamplerDesc = {};
	defaultSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	defaultSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	defaultSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	defaultSamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_ANISOTROPIC;
	defaultSamplerDesc.MinLOD = 0;
	defaultSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	defaultSamplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
	if (FAILED(device->CreateSamplerState(&defaultSamplerDesc, &defaultSampler)))
	{
		MessageBoxA(NULL, "Could not create sampler state", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	D3D11_SAMPLER_DESC computeSamplerDesc = {};
	computeSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	computeSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	computeSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	computeSamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	computeSamplerDesc.MinLOD = 0;
	computeSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	computeSamplerDesc.MaxAnisotropy = 1;
	if (FAILED(device->CreateSamplerState(&computeSamplerDesc, &computeSampler)))
	{
		MessageBoxA(NULL, "Could not create sampler state", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	D3D11_RENDER_TARGET_BLEND_DESC targetDesc = {};
	targetDesc.BlendEnable = true;
	targetDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	targetDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	targetDesc.BlendOp = D3D11_BLEND_OP_ADD;
	targetDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	targetDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	targetDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	targetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.RenderTarget[0] = targetDesc;
	if (FAILED(device->CreateBlendState(&blendDesc, &blendState)))
	{
		MessageBoxA(NULL, "Could not create blend State", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
#ifdef ENGINE_COMPILE_DEBUG
	if (FAILED(device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug))))
	{
		printf("Failed to query for debug interface.\n");
		return false;
	}
#endif
	//GenerateQuad();
	CreateShader();
	return true;
}

void Engine::Graphics::D3D11Renderer::SetViewPort()
{
	D3D11_VIEWPORT viewport
	{
		0.0f,
		0.0f,
		static_cast<FLOAT>(width),
		static_cast<FLOAT>(height),
		0.0f,
		1.0f
	};
	context->RSSetViewports(1, &viewport);
}

bool Engine::Graphics::D3D11Renderer::CreateRenderTarget()
{
	SAFERELEASE(rtv);
	//Create a texture2D that will represent the screen image
	ID3D11Texture2D* surface = nullptr;
	if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&surface))))
	{
		MessageBoxA(NULL, "Could not get back-buffer interface", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	//Create a render target view from the texture2D we just created
	D3D11_RENDER_TARGET_VIEW_DESC RTVdesc = {};
	if (FAILED(device->CreateRenderTargetView(surface, 0, &rtv)))
	{
		MessageBoxA(NULL, "Could not create render target view", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	//Release the texture2D as we assigned it to the render target view
	SAFERELEASE(surface);
	return true;
}

bool Engine::Graphics::D3D11Renderer::CreateDepthStencil()
{
	SAFERELEASE(depthView);
	//Create a Texture2D with 1 channel that the depth stencil buffer can write to
	ID3D11Texture2D* depthSurface = reinterpret_cast<ID3D11Texture2D*>(CreateTexture(width, height, 1, TextureFormat::D32));
	if (depthSurface == nullptr)
	{
		MessageBoxA(NULL, "Could not create depth buffer texture", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	//Create the depth stencil state that will descripe how depth is calculate if enabled
	D3D11_DEPTH_STENCIL_DESC stencilDesc = {};
	stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	stencilDesc.DepthEnable = true;
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stencilDesc.StencilEnable = false;
	ID3D11DepthStencilState* depthState = nullptr;
	if (FAILED(device->CreateDepthStencilState(&stencilDesc, &depthState)))
	{
		MessageBoxA(NULL, "Could not create depth buffer state", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	context->OMSetDepthStencilState(depthState, 1);

	//Create the depth view from the Texture
	D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {};
	depthViewDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	depthViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthViewDesc.Texture2D.MipSlice = 0;
	if (FAILED(device->CreateDepthStencilView(depthSurface, &depthViewDesc, &depthView)))
	{
		MessageBoxA(NULL, "Could not create depth view", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	SAFERELEASE(depthSurface);
	return true;
}

bool Engine::Graphics::D3D11Renderer::Resize(ui32 width, ui32 height)
{
	context->OMSetRenderTargets(0, 0, 0);
	SAFERELEASE(rtv);
	SAFERELEASE(depthView);

	this->width = width;
	this->height = height;
	if (FAILED(swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)))
	{
		MessageBoxA(NULL, "Could not resize swapChain", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	CreateDepthStencil();
	CreateRenderTarget();

	SetViewPort();

	return true;
}

void D3D11Renderer::CreateShader()
{
	/*std::vector<ui32> vertexShaderSource;
	FILE* vertexShaderSourceFile = fopen("./data/shd/bsdfVertex.spirv", "r");
	if (vertexShaderSourceFile == nullptr)
	{
		throw std::string("bsdfVertex shader now found");
	}
	ui32 c;
	while ((c = fgetc(vertexShaderSourceFile)) != EOF)
	{
		vertexShaderSource.push_back(c);
	}
	spirv_cross::CompilerHLSL hlslCompiler(std::move(vertexShaderSource));

	spirv_cross::CompilerHLSL::Options options;

	hlslCompiler.set_hlsl_options(options);

	std::string vertexShaderHLSLSource = hlslCompiler.compile();*/
	//Load compiled Vertex Shader file to a blob
	ID3DBlob* vertexShaderBlob = nullptr;
	if (FAILED(D3DReadFileToBlob(L"./data/shd/bsdfVertex.shader", &vertexShaderBlob)))
	{
		MessageBoxA(NULL, "Could not load vertex shader", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Load compiled Pixel Shader file to a blob
	ID3DBlob* pixelShaderBlob = nullptr;
	if (FAILED(D3DReadFileToBlob(L"./data/shd/bsdfPixel.shader", &pixelShaderBlob)))
	{
		MessageBoxA(NULL, "Could not load pixel shader", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}//Load compiled Pixel Shader file to a blob
	ID3DBlob* pixelSDFShaderBlob = nullptr;
	if (FAILED(D3DReadFileToBlob(L"./data/shd/pixelSDFDefault.shader", &pixelSDFShaderBlob)))
	{
		MessageBoxA(NULL, "Could not load pixel shader", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Create a Vertex Shader from blob
	if (FAILED(device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader)))
	{
		MessageBoxA(NULL, "Could not create pixel shader", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Create a Pixel Shader from blob
	if (FAILED(device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader)))
	{
		MessageBoxA(NULL, "Could not create pixel shader", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Create a Pixel Shader from blob
	if (FAILED(device->CreatePixelShader(pixelSDFShaderBlob->GetBufferPointer(), pixelSDFShaderBlob->GetBufferSize(), nullptr, &pixelSDFShader)))
	{
		MessageBoxA(NULL, "Could not create pixel SDF shader", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Create Input Elements for the vertex Shader
	D3D11_INPUT_ELEMENT_DESC vertexElemDesc[5] = { };

	/*
	D3D11_INPUT_PER_VERTEX_DATA to declare that these elements are for a vertex shader
	D3D11_APPEND_ALIGNED_ELEMENT to append the element next to each other at the end of each data segment
	SemanticIndex -> declare which of the shematic with the name SemanticName is used e.g.
	SemanticIndex = 0
	SemanticName = "POSITION"
	 -> POSITOON0
	AlignedByteOffset if the data need to be aligned with a specific offset
	*/
	//Format for 3 Float for the position vec3
	vertexElemDesc[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[0].InputSlot = 0;
	vertexElemDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertexElemDesc[0].AlignedByteOffset = 0;
	vertexElemDesc[0].SemanticIndex = 0;
	vertexElemDesc[0].SemanticName = "POSITION";
	//Format for 3 Float for the normal vec3
	vertexElemDesc[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[1].InputSlot = 0;
	vertexElemDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertexElemDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[1].SemanticIndex = 0;
	vertexElemDesc[1].SemanticName = "NORMAL";
	//Format for 2 Float for the texcoord/uv vec2
	vertexElemDesc[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	vertexElemDesc[2].InputSlot = 0;
	vertexElemDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertexElemDesc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[2].SemanticIndex = 0;
	vertexElemDesc[2].SemanticName = "TEXCOORD";

	vertexElemDesc[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[3].InputSlot = 0;
	vertexElemDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertexElemDesc[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[3].SemanticIndex = 0;
	vertexElemDesc[3].SemanticName = "TANGENT";

	vertexElemDesc[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[4].InputSlot = 0;
	vertexElemDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertexElemDesc[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[4].SemanticIndex = 0;
	vertexElemDesc[4].SemanticName = "BITTANGENT";

	//Create a Input Layout for the vertex shader with the descriptions and count of the elements using the data read into the blob
	if (FAILED(device->CreateInputLayout(vertexElemDesc, sizeof(vertexElemDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &vertexLayout)))
	{
		MessageBoxA(NULL, "Could not create vertex input layout", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//No need to create Input Elements for the pixel shader as the vertex shader give data to it in the shader code
}

void D3D11Renderer::BeginScene()
{
	context->ClearRenderTargetView(rtv, clearColor);
	context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	worldLocalBuffer.lightCount = static_cast<ui32>(lights.size());
	int i = 0;
	for (GpuLight l : lights)
	{
		worldLocalBuffer.lights[i] = l;
		i++;
	}
	if (worldBuffer != nullptr)
	{
		D3D11_MAPPED_SUBRESOURCE camResource = {};
		if (FAILED(context->Map(reinterpret_cast<ID3D11Resource*>(worldBuffer), 0, D3D11_MAP_WRITE_DISCARD, 0, &camResource)))
		{
			MessageBoxA(NULL, "could not map transform buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		//Convert mapped data to desired type
		worldConstant* dataMat = reinterpret_cast<worldConstant*>(camResource.pData);

		//Assign new data to be uploaded
		if (dataMat)
		{
			(*dataMat) = worldLocalBuffer;
		}
		//Unmap to confirm upload and discard old data
		context->Unmap(reinterpret_cast<ID3D11Resource*>(worldBuffer), 0);
	}
	else
	{
		worldBuffer = CreateBuffer(BufferType::Constant, &worldLocalBuffer, sizeof(worldConstant), UsageType::Dynamic);
		if (!worldBuffer)
		{
			MessageBoxA(NULL, "Could not set camera", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		}
	}
	//if (Camera::activeCamera)
	//{
	//	Camera::activeCamera->SetAspect(static_cast<real>(width) / static_cast<real>(height));
	//	//Map the contents of the cameraBuffer in CPU memory to be accessed by CPU
	//	D3D11_MAPPED_SUBRESOURCE camResource = {};
	//	if (FAILED(context->Map(reinterpret_cast<ID3D11Resource*>(worldBuffer), 0, D3D11_MAP_WRITE_DISCARD, 0, &camResource)))
	//	{
	//		MessageBoxA(NULL, "could not map transform buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
	//		return;
	//	}
	//	//Convert mapped data to desired type
	//	worldConstant* dataMat = reinterpret_cast<worldConstant*>(camResource.pData);
	//
	//	//Assign new data to be uploaded
	//	if (dataMat)
	//	{
	//		//(*dataMat).lights;
	//		dataMat->projView = Camera::activeCamera->GetViewProjMatrix();
	//		dataMat->eye = Camera::activeCamera->gameObject->transform->position;
	//		dataMat->ambientLight = 0.2f;
	//	}
	//	//Unmap to confirm upload and discard old data
	//	context->Unmap(reinterpret_cast<ID3D11Resource*>(worldBuffer), 0);
	//}
}

void D3D11Renderer::EndScene()
{
	if (vsyncEnable)
		swapChain->Present(1, 0);
	else
		swapChain->Present(0, 0);
	
}

void D3D11Renderer::Shutdown()
{
	//exit fullscreen mode if we are in it
	if (swapChain)
		swapChain->SetFullscreenState(false, nullptr);
	
#ifdef ENGINE_COMPILE_DEBUG
	if (FAILED(this->debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL)))
	{
		printf("Failed to activate report of pending graphics objects.\n");
	}

	SAFERELEASE(this->debug);
#endif
	//Delete / Release in order of creation
	SAFERELEASE(rtv);
	SAFERELEASE(context);
	SAFERELEASE(device);
	SAFERELEASE(swapChain);

	SAFERELEASE(vertexLayout);
	SAFERELEASE(vertexShader);
	SAFERELEASE(pixelShader);
	SAFERELEASE(defaultSampler);
	SAFERELEASE(computeSampler);
}

void D3D11Renderer::Render(Engine::Math::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount)
{
	//INPUT ASSEMBLER STAGE

	ui32 stride = sizeof(Vertex);
	ui32 offset = 0;
	//Generate model Matrix from Position/Translation Rotation Scale -> TRS
	
	//Map the contents of the transformBuffer in CPU memory to be accessed by CPU
	D3D11_MAPPED_SUBRESOURCE modelResource = {};
	if (FAILED(context->Map(reinterpret_cast<ID3D11Resource*>(modelBuffer), 0, D3D11_MAP_WRITE_DISCARD, 0, &modelResource)))
	{
		MessageBoxA(NULL, "could not map transform buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Convert mapped data to desired type
	modelConstant* dataMat = reinterpret_cast<modelConstant*>(modelResource.pData);
	//Assign new data to be uploaded
	if (dataMat)
	{
		dataMat->world = transformMat;
	}
	
	//Unmap to confirm upload and discard old data
	context->Unmap(reinterpret_cast<ID3D11Resource*>(modelBuffer), 0);
	
	
	//Bind the vertex data that describes the object we want to render
	context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vertexBuffer), &stride, &offset);
	//Bind the index data that describes the faces of the object we want to render
	context->IASetIndexBuffer(reinterpret_cast<ID3D11Buffer*>(indexBuffer), DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
	//The the assembler what type of data he will be working with
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//Tell the assembler how the vertex shader inputs look like
	context->IASetInputLayout(vertexLayout);
	
	//VERTEX SHADER STAGE
	//Bind the transformation for rendering this object
	context->VSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&modelBuffer));
	//Bind the camera transformation for rendering this object
	context->VSSetConstantBuffers(1, 1, reinterpret_cast<ID3D11Buffer**>(&worldBuffer));
	context->VSSetShader(vertexShader, nullptr, 0);
	
	//PIXEL SHADER STAGE
	//Bind the transformation for rendering this object
	context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&modelBuffer));
	//Bind the camera transformation for rendering this object
	context->PSSetConstantBuffers(1, 1, reinterpret_cast<ID3D11Buffer**>(&worldBuffer));
	
		//if (renderer->texture != nullptr)
		//{
		//	context->PSSetSamplers(0, 1, &sampler);
		//	context->PSSetShaderResources(0, 1, &renderer->texture);
		//}
		//context->PSSetShader(pixelSDFShader, nullptr, 0);
	context->PSSetShaderResources(0, static_cast<ui32>(textureViews.size()), textureViews.data());
	ID3D11SamplerState* const modelSamplers[] =
	{
		defaultSampler,
		computeSampler
	};
	//context->PSGetShaderResources(0, )
	context->PSSetSamplers(0, 2, modelSamplers);
	context->PSSetShader(pixelShader, nullptr, 0);
	//OUTPUT MERGER STAGE
	
	context->OMSetRenderTargets(1, &rtv, depthView);
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(blendState, blendFactor, 0xffffffff);
	//Draw the object with the amount indices the object has
	context->DrawIndexed(indexCount, 0, 0);
	textureViews.clear();
}
void D3D11Renderer::SetActiveCamera(Vec3 eye, Mat4x4 viewProj)
{
	worldLocalBuffer.eye = eye;
	worldLocalBuffer.projView = viewProj;
}

void Engine::Graphics::D3D11Renderer::ClearLights()
{
	lights.clear();
}

void Engine::Graphics::D3D11Renderer::SetLight(Engine::Utils::GpuLight lightDescriptor)
{
	lights.push_back(lightDescriptor);
}

bool Engine::Graphics::D3D11Renderer::CheckForFullscreen()
{
	BOOL fullscreen;
	swapChain->GetFullscreenState(&fullscreen, nullptr);
	if (fullscreen != inFullscreen)
	{
		inFullscreen = fullscreen;
		return true;
	}
	return false;
}

IntPtr Engine::Graphics::D3D11Renderer::CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data)
{
	ui32 pitch = 0;
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = levels;
	desc.ArraySize = 1;
	switch (format)
	{
	case TextureFormat::RGBAFLOAT:
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		pitch = width * sizeof(float) * 4U;
		break;
	case TextureFormat::RGBA32:
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pitch = width * sizeof(byte) * 4U;
		break;
	case TextureFormat::D32:
		desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		pitch = width * sizeof(byte);
		break;
	case TextureFormat::ALPHA8:
		desc.Format = DXGI_FORMAT_A8_UNORM;
		pitch = width * sizeof(byte);
		break;
	case TextureFormat::RED8:
		desc.Format = DXGI_FORMAT_R8_UNORM;
		pitch = width * sizeof(byte);
		break;
	case TextureFormat::REDFLOAT:
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		pitch = width * sizeof(float);
		break;
	case TextureFormat::RED1:
		desc.Format = DXGI_FORMAT_R1_UNORM;
		pitch = width * sizeof(bool);
	}
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	if (format == TextureFormat::D32)
	{
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	}
	else
	{
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		if (levels == 0) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}
	}
	D3D11_SUBRESOURCE_DATA* bufferSubResource = nullptr;
	if (data)
	{
		bufferSubResource = new D3D11_SUBRESOURCE_DATA();
		bufferSubResource->pSysMem = data;
		bufferSubResource->SysMemPitch = pitch;
		bufferSubResource->SysMemSlicePitch = height * pitch;

	}
	
	ID3D11Texture2D* tex2D = nullptr;
	if (FAILED(device->CreateTexture2D(&desc, bufferSubResource, &tex2D)))
	{
		MessageBoxA(NULL, "Could not create texture2D", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return nullptr;
	}
	return reinterpret_cast<IntPtr>(tex2D);
}

void Engine::Graphics::D3D11Renderer::ReleaseTexture(IntPtr& texture)
{
	ID3D11Texture2D* tex = reinterpret_cast<ID3D11Texture2D*>(texture);
	SAFERELEASE(tex);
	texture = nullptr;
}

void Engine::Graphics::D3D11Renderer::UseTexture(ui32 slot, GraphicsBufferPtr view)
{
	while (slot >= textureViews.size())
	{
		textureViews.push_back(nullptr);
	}
	textureViews[slot] = reinterpret_cast<ID3D11ShaderResourceView*>(view);
	
}

IntPtr Engine::Graphics::D3D11Renderer::CreateTextureSRV(IntPtr texture, TextureFormat format)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	switch (format)
	{
	case TextureFormat::RGBAFLOAT:
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case TextureFormat::RGBA32:
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case TextureFormat::D32:
		srvDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		break;
	}
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	ID3D11ShaderResourceView* srv = nullptr;
	if (FAILED(device->CreateShaderResourceView(reinterpret_cast<ID3D11Texture2D*>(texture), &srvDesc, &srv)))
	{
		return nullptr;
	}
	return reinterpret_cast<IntPtr>(srv);
}

void Engine::Graphics::D3D11Renderer::ReleaseTextureSRV(IntPtr& srv)
{
	ID3D11ShaderResourceView* tex = reinterpret_cast<ID3D11ShaderResourceView*>(srv);
	SAFERELEASE(tex);
	srv = nullptr;
}

GraphicsBufferPtr D3D11Renderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	switch (usage)
	{
	case UsageType::Dynamic:
		bufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
		break;
	default:
		bufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
		break;
	}
	bufferDesc.ByteWidth = dataSize;

	switch (type)
	{
	case BufferType::Vertex:
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		break;
	case BufferType::Index:
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		break;
	case BufferType::Constant:
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		break;
	default:
		MessageBoxA(NULL, "invalid buffer type", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return nullptr;
	}
	if(type == BufferType::Constant)
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	else
		bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	/*
	Create Vertex Buffer Resource
	pSysMem the pointer to the data to be uploaded to the buffer
	*/
	D3D11_SUBRESOURCE_DATA bufferSubResource = {};
	bufferSubResource.pSysMem = data;
	bufferSubResource.SysMemPitch = 0;
	bufferSubResource.SysMemSlicePitch = 0;

	ID3D11Buffer* buffer = nullptr;
	//Tell device to create Vertex Buffer and the data in subresource
	if (FAILED(device->CreateBuffer(&bufferDesc, &bufferSubResource, &buffer)))
	{
		MessageBoxA(NULL, "Could not create vertex buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return nullptr;
	}
	return reinterpret_cast<GraphicsBufferPtr>(buffer);
}

void Engine::Graphics::D3D11Renderer::ReleaseBuffer(IntPtr& buffer)
{
	ID3D11Buffer* b = reinterpret_cast<ID3D11Buffer*>(buffer);
	SAFERELEASE(b);
	buffer = nullptr;
}
