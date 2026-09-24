
// EXTERNAL INCLUDES

#include <iostream>
#include <algorithm>
#define NOMINMAX
#include <d3dcompiler.h>
#include <SDL.h>
#include <SDL_syswm.h>
// INTERNAL INCLUDES
#include "d3d11renderer.h"
#include "utils/util.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "utils/color.h"
#include "vertex.h"
#include "spirv_cpp.hpp"
#include "spirv_parser.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_reflect.hpp"
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_dx11.h>
#include "log.h"
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;
using namespace DUPLEX_NS_GRAPHICS;
using namespace DUPLEX_NS_LOG;

namespace
{
	// res/CMakeLists.txt compiles the shared .spirv with -fvk-t-shift 10 0 -fvk-s-shift 20 0, so
	// every SRV's Vulkan binding sits at +10 and every sampler's at +20 - purely to keep them out
	// of the way of cbuffers b0/b1 in a single Vulkan descriptor set (see that file's own
	// comment). SPIRV-Cross's HLSL backend otherwise preserves those binding numbers verbatim as
	// HLSL register numbers, which both overshoots HLSL SM5.0's 16-slot sampler limit (a bound
	// sampler landing on s20 fails to compile outright - confirmed directly: D3DCompile fails
	// with "maximum sampler register index exceeded, target has 16 slots") and disagrees with
	// where this renderer actually binds resources on the CPU side (PSSetShaderResources(0, ...)/
	// PSSetSamplers(0, ...) - see Render()). This undoes exactly that shift so the cross-compiled
	// HLSL ends up back at the registers the original res/ps/*.hlsl source declared.
	void RemapVulkanShiftedBindingsToHlsl(spirv_cross::CompilerHLSL& compiler)
	{
		// add_hlsl_resource_binding() keys its remap table by (stage, desc_set, binding) - stage
		// defaults to spv::ExecutionModelMax on a fresh HLSLResourceBinding, but
		// remap_hlsl_resource_binding() looks entries up by this compiler's *actual*
		// get_execution_model() (Vertex/Fragment/...), so a remap left at the default stage
		// silently never matches and is never applied. Must be set explicitly per compiler
		// instance (confirmed by testing: omitting this left every SRV/sampler register
		// unremapped, still landing on the original Vulkan-shifted binding number).
		spv::ExecutionModel stage = compiler.get_execution_model();
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();
		for (const spirv_cross::Resource& res : resources.separate_images)
		{
			ui32 binding = compiler.get_decoration(res.id, spv::DecorationBinding);
			spirv_cross::HLSLResourceBinding remap;
			remap.stage = stage;
			remap.desc_set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
			remap.binding = binding;
			remap.srv.register_space = 0;
			remap.srv.register_binding = binding - 10;
			compiler.add_hlsl_resource_binding(remap);
		}
		for (const spirv_cross::Resource& res : resources.separate_samplers)
		{
			ui32 binding = compiler.get_decoration(res.id, spv::DecorationBinding);
			spirv_cross::HLSLResourceBinding remap;
			remap.stage = stage;
			remap.desc_set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
			remap.binding = binding;
			remap.sampler.register_space = 0;
			remap.sampler.register_binding = binding - 20;
			compiler.add_hlsl_resource_binding(remap);
		}
	}
}

bool D3D11Renderer::Init(SDL_Window* window, ui32 width, ui32 height)
{
	this->width = width;
	this->height = height;

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(window, &wmInfo))
	{
		MessageBoxA(NULL, "Could not get native window handle from SDL", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	HWND hwnd = wmInfo.info.win.window;

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
		Logger::Renderer().info("Graphics Device: {}", adapterDescText);
		Logger::Renderer().info("Graphics available Memory: {} MB", static_cast<ui32>(adapterDesc.DedicatedVideoMemory * 9.5367E-7f));
	}
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
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
	swapChainDesc.OutputWindow = hwnd;
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
	if (!modelBuffer.IsValid())
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
		Logger::Renderer().warn("Failed to query for debug interface.");
		return false;
	}
#endif
	//GenerateQuad();
	CreateShader();

	// See defaultTextureHandle's declaration - matches D3D12Renderer/VulkanRenderer, which both
	// already had this.
	ui32 white = 0xFFFFFFFFu;
	defaultTextureHandle = CreateTexture(1, 1, 1, TextureFormat::RGBA32, &white);
	if (!defaultTextureHandle.IsValid())
	{
		MessageBoxA(NULL, "Could not create default texture", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	defaultTextureView = CreateTextureSRV(defaultTextureHandle, TextureFormat::RGBA32);
	if (!defaultTextureView.IsValid())
	{
		MessageBoxA(NULL, "Could not create default texture SRV", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	Logger::Renderer().info("D3D11Renderer initialized ({}x{})", width, height);
	return true;
}

void D3D11Renderer::SetViewPort()
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

bool D3D11Renderer::CreateRenderTarget()
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

bool D3D11Renderer::CreateDepthStencil()
{
	SAFERELEASE(depthView);
	//Create a Texture2D with 1 channel that the depth stencil buffer can write to
	TextureHandle depthTextureHandle = CreateTexture(width, height, 1, TextureFormat::D32);
	if (!depthTextureHandle.IsValid())
	{
		MessageBoxA(NULL, "Could not create depth buffer texture", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	ID3D11Texture2D* depthSurface = texturePool.Get(depthTextureHandle);
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
	// Goes through ReleaseTexture (not a raw SAFERELEASE) so the texturePool slot is freed
	// too - previously this released the COM object directly, leaving a stale "alive" entry
	// in the pool with a dangling pointer.
	ReleaseTexture(depthTextureHandle);
	return true;
}

bool D3D11Renderer::Resize(ui32 width, ui32 height)
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
	ID3DBlob* vertexShaderBlob = nullptr;
	ID3DBlob* pixelShaderBlob = nullptr;

	{
		FILE* file = fopen("./bin/data/shd/bsdfVertex.spirv", "rb");
		if (file == nullptr)
		{
			// Was an uncaught throw std::string(...) - nothing anywhere catches exceptions from
			// this deep in Init(), so it crashed silently instead of explaining itself. This is
			// the exact failure a wrong working directory produces (e.g. double-clicking the exe
			// in Explorer, which sets CWD to its own folder, not the repo root these ./bin/...
			// paths assume) - confirmed directly by reproducing it.
			MessageBoxA(NULL, "Could not find ./bin/data/shd/bsdfVertex.spirv - make sure the working directory is the repo root (e.g. run from a terminal cd'd there), not wherever the .exe itself lives.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		fseek(file, 0, SEEK_END);
		long len = ftell(file) / sizeof(ui32);
		rewind(file);
		std::vector<ui32> spirv(len);
		if (fread(spirv.data(), sizeof(ui32), len, file) != size_t(len))
			spirv.clear();
		fclose(file);
		spirv_cross::Parser parser(std::move(spirv));

		parser.parse();
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 50;
		spirv_cross::CompilerHLSL hlslCompiler(parser.get_parsed_ir());
		hlslCompiler.set_hlsl_options(options);
		hlslCompiler.add_vertex_attribute_remap({ 0, "POSITION0" });
		hlslCompiler.add_vertex_attribute_remap({ 1, "NORMAL0" });
		hlslCompiler.add_vertex_attribute_remap({ 2, "TEXCOORD0" });
		hlslCompiler.add_vertex_attribute_remap({ 3, "TANGENT0" });
		hlslCompiler.add_vertex_attribute_remap({ 4, "BITTANGENT0" });
		hlslCompiler.add_vertex_attribute_remap({ 5, "BITTANGENT0" });
		RemapVulkanShiftedBindingsToHlsl(hlslCompiler);
		std::string vertexShaderHLSLSource = hlslCompiler.compile();
		auto model = hlslCompiler.get_execution_model();
		D3D_SHADER_MACRO defines[] =
		{
			NULL, NULL
		};
		ID3DBlob* errorBlob = nullptr;
		HRESULT res = D3DCompile(vertexShaderHLSLSource.c_str(), vertexShaderHLSLSource.length(), nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, NULL, &vertexShaderBlob, &errorBlob);
		if (FAILED(res))
		{
			// Previously fell through here (only logging via a MessageBox and continuing) -
			// vertexShaderBlob stays null on a failed compile, so every use of it below
			// (CreateVertexShader/CreateInputLayout) was a guaranteed null-pointer crash rather
			// than a clean, diagnosable failure.
			if (errorBlob)
			{
				MessageBoxA(NULL, static_cast<char*>(errorBlob->GetBufferPointer()), "Vertex shader compile failed", MB_OK | MB_ICONEXCLAMATION);
				errorBlob->Release();
			}
			return;
		}
	}
	{
		FILE* file = fopen("./bin/data/shd/bsdfPixel.spirv", "rb");
		if (file == nullptr)
		{
			// See the bsdfVertex block above for why this is a MessageBoxA + return, not a throw.
			MessageBoxA(NULL, "Could not find ./bin/data/shd/bsdfPixel.spirv - make sure the working directory is the repo root, not wherever the .exe itself lives.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			SAFERELEASE(vertexShaderBlob);
			return;
		}
		fseek(file, 0, SEEK_END);
		long len = ftell(file) / sizeof(ui32);
		rewind(file);
		std::vector<ui32> spirv(len);
		if (fread(spirv.data(), sizeof(ui32), len, file) != size_t(len))
			spirv.clear();
		fclose(file);
		spirv_cross::Parser parser(std::move(spirv));
		parser.parse();
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 50;

		spirv_cross::CompilerHLSL hlslCompiler(parser.get_parsed_ir());
		hlslCompiler.set_hlsl_options(options);
		RemapVulkanShiftedBindingsToHlsl(hlslCompiler);
		std::string pixelShaderHLSLSource = hlslCompiler.compile();

		D3D_SHADER_MACRO defines[] =
		{
			NULL, NULL
		};
		ID3DBlob* errorBlob = nullptr;
		HRESULT res = D3DCompile(pixelShaderHLSLSource.c_str(), pixelShaderHLSLSource.length(), nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, NULL, &pixelShaderBlob, &errorBlob);
		if (FAILED(res))
		{
			// See the vertex shader block above - falling through here left pixelShaderBlob
			// null, guaranteeing a later crash instead of a diagnosable failure.
			if (errorBlob)
			{
				MessageBoxA(NULL, static_cast<char*>(errorBlob->GetBufferPointer()), "Pixel shader compile failed", MB_OK | MB_ICONEXCLAMATION);
				errorBlob->Release();
			}
			SAFERELEASE(vertexShaderBlob);
			return;
		}
	}
	// pixelSDFDefault.hlsl is never actually bound anywhere (the only reference to
	// pixelSDFShader in Render() is commented out) - matches VulkanRenderer's CreateShader(),
	// which never loads it either. Not compiled here either: unlike bsdfPixel.hlsl, its
	// g_texture/textureSampler have no explicit register() declarations, so dxc's implicit
	// binding assignment (before res/CMakeLists.txt's -fvk-t-shift/-fvk-s-shift are even applied)
	// produces a SPIR-V binding SPIRV-Cross can't turn back into a valid HLSL register - a
	// pre-existing gap in that shader asset, not something worth fixing for a shader nothing
	// draws with.

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

	//Generate LUTs

}

void D3D11Renderer::BeginScene()
{
	context->ClearRenderTargetView(rtv, clearColor);
	context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	// Also (redundantly) bound in Render() before each draw - kept there unchanged, but ImGui
	// needs this bound even when nothing calls Render() at all (see engine.editor, which draws
	// no meshes), so it can't rely on that happening implicitly.
	context->OMSetRenderTargets(1, &rtv, depthView);

	worldLocalBuffer.lightCount = static_cast<ui32>(lights.size());
	int i = 0;
	for (GpuLight& l : lights)
	{
		worldLocalBuffer.lights[i] = l;
		i++;
	}
	if (worldBuffer.IsValid())
	{
		ID3D11Buffer* worldBufferPtr = bufferPool.Get(worldBuffer);
		D3D11_MAPPED_SUBRESOURCE camResource = {};
		if (FAILED(context->Map(worldBufferPtr, 0, D3D11_MAP_WRITE_DISCARD, 0, &camResource)))
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
		context->Unmap(worldBufferPtr, 0);
	}
	else
	{
		worldBuffer = CreateBuffer(BufferType::Constant, &worldLocalBuffer, sizeof(worldConstant), UsageType::Dynamic);
		if (!worldBuffer.IsValid())
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
	Logger::Renderer().info("D3D11Renderer shutting down");

	if (imguiInitialized)
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		imguiInitialized = false;
	}

	//exit fullscreen mode if we are in it
	if (swapChain)
		swapChain->SetFullscreenState(false, nullptr);

#ifdef ENGINE_COMPILE_DEBUG
	if (FAILED(this->debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL)))
	{
		Logger::Renderer().warn("Failed to activate report of pending graphics objects.");
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

void D3D11Renderer::Render(Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount)
{
	//INPUT ASSEMBLER STAGE

	ui32 stride = sizeof(Vertex);
	ui32 offset = 0;
	//Generate model Matrix from Position/Translation Rotation Scale -> TRS

	ID3D11Buffer* modelBufferPtr = bufferPool.Get(modelBuffer);
	ID3D11Buffer* worldBufferPtr = bufferPool.Get(worldBuffer);
	ID3D11Buffer* vertexBufferPtr = bufferPool.Get(vertexBuffer);
	ID3D11Buffer* indexBufferPtr = bufferPool.Get(indexBuffer);

	//Map the contents of the transformBuffer in CPU memory to be accessed by CPU
	D3D11_MAPPED_SUBRESOURCE modelResource = {};
	if (FAILED(context->Map(modelBufferPtr, 0, D3D11_MAP_WRITE_DISCARD, 0, &modelResource)))
	{
		MessageBoxA(NULL, "could not map transform buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//Convert mapped data to desired type
	modelConstant* dataMat = reinterpret_cast<modelConstant*>(modelResource.pData);
	//Assign new data to be uploaded
	if (dataMat)
	{
		dataMat->world = ToShaderLayout(transformMat);
	}

	//Unmap to confirm upload and discard old data
	context->Unmap(modelBufferPtr, 0);


	//Bind the vertex data that describes the object we want to render
	context->IASetVertexBuffers(0, 1, &vertexBufferPtr, &stride, &offset);
	//Bind the index data that describes the faces of the object we want to render
	context->IASetIndexBuffer(indexBufferPtr, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
	//The the assembler what type of data he will be working with
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//Tell the assembler how the vertex shader inputs look like
	context->IASetInputLayout(vertexLayout);

	//VERTEX SHADER STAGE
	//Bind the transformation for rendering this object
	context->VSSetConstantBuffers(0, 1, &modelBufferPtr);
	//Bind the camera transformation for rendering this object
	context->VSSetConstantBuffers(1, 1, &worldBufferPtr);
	context->VSSetShader(vertexShader, nullptr, 0);

	//PIXEL SHADER STAGE
	//Bind the transformation for rendering this object
	context->PSSetConstantBuffers(0, 1, &modelBufferPtr);
	//Bind the camera transformation for rendering this object
	context->PSSetConstantBuffers(1, 1, &worldBufferPtr);
	
		//if (renderer->texture != nullptr)
		//{
		//	context->PSSetSamplers(0, 1, &sampler);
		//	context->PSSetShaderResources(0, 1, &renderer->texture);
		//}
		//context->PSSetShader(pixelSDFShader, nullptr, 0);
	// Was textureViews.size() - for a draw with no material (see meshsystem.h: UseTexture() is
	// only called when the MeshRenderer actually has one), textureViews had just been cleared to
	// empty by the previous draw's cleanup below, so this bound zero resources. D3D11 treats
	// PSSetShaderResources(slot, 0, ...) as a no-op, not an unbind - it silently left whatever
	// the PREVIOUS draw call's textures were still bound, so an unmaterialed object rendered
	// wearing the last-drawn material's textures instead of the intended plain/untextured look.
	// Confirmed directly: the physics demo's floor/boxes (no material) rendered "textured" with
	// the BoomBox's own textures leaking over from the draw call right before them in the same
	// frame - and only on this backend, since D3D12Renderer/VulkanRenderer both write a full,
	// fresh set of texture bindings (falling back to a default texture) on every single draw
	// regardless of whether UseTexture() was called. Binding a fixed 6 slots (matching
	// bsdfPixel.hlsl's t0-t5) rather than textureViews' current size, padded below with
	// defaultTextureView rather than null (see its declaration), reproduces that same
	// per-draw-fresh behavior here.
	ID3D11ShaderResourceView* defaultSrv = srvPool.Get(defaultTextureView);
	while (textureViews.size() < 6)
	{
		textureViews.push_back(defaultSrv);
	}
	context->PSSetShaderResources(0, 6, textureViews.data());
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
	// Reset to the default texture rather than clear() (which would shrink back to empty,
	// reintroducing the bug this whole block exists to fix - see above) so the *next* draw call,
	// even one with no material at all, still binds 6 explicit default-texture views instead of
	// leaking this draw's bindings forward.
	std::fill(textureViews.begin(), textureViews.end(), defaultSrv);
}
void D3D11Renderer::SetActiveCamera(Vec3 eye, Mat4x4 viewProj)
{
	worldLocalBuffer.eye = eye;
	worldLocalBuffer.projView = ToShaderLayout(viewProj);
}

void D3D11Renderer::ClearLights()
{
	lights.clear();
}

void D3D11Renderer::SetLight(GpuLight lightDescriptor)
{
	lightDescriptor.transform = ToShaderLayout(lightDescriptor.transform);
	lights.push_back(lightDescriptor);
}

TextureHandle D3D11Renderer::CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data)
{
	ui32 pitch = PixelSizeFromTextureFormat(format) * width;
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = levels;
	desc.ArraySize = 1;
	desc.Format = FromTextureFormat(format);
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
		return TextureHandle{};
	}
	return texturePool.Create(tex2D);
}

void D3D11Renderer::ReleaseTexture(TextureHandle& texture)
{
	if (!texture.IsValid())
		return;
	ID3D11Texture2D* tex = texturePool.Get(texture);
	SAFERELEASE(tex);
	texturePool.Release(texture);
}

void D3D11Renderer::UseTexture(ui32 slot, ShaderResourceViewHandle view)
{
	ID3D11ShaderResourceView* defaultSrv = srvPool.Get(defaultTextureView);
	while (slot >= textureViews.size())
	{
		textureViews.push_back(defaultSrv);
	}
	textureViews[slot] = view.IsValid() ? srvPool.Get(view) : defaultSrv;
}

ShaderResourceViewHandle D3D11Renderer::CreateTextureSRV(TextureHandle texture, TextureFormat format)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = FromTextureFormat(format);
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	ID3D11ShaderResourceView* srv = nullptr;
	if (FAILED(device->CreateShaderResourceView(texturePool.Get(texture), &srvDesc, &srv)))
	{
		return ShaderResourceViewHandle{};
	}
	return srvPool.Create(srv);
}

void D3D11Renderer::ReleaseTextureSRV(ShaderResourceViewHandle& srv)
{
	if (!srv.IsValid())
		return;
	ID3D11ShaderResourceView* view = srvPool.Get(srv);
	SAFERELEASE(view);
	srvPool.Release(srv);
}

ShaderResourceViewHandle D3D11Renderer::CreateCubemapSRV(TextureHandle cubemap, TextureFormat format)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = FromTextureFormat(format);
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D10_1_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;
	ID3D11ShaderResourceView* srv = nullptr;
	if (FAILED(device->CreateShaderResourceView(texturePool.Get(cubemap), &srvDesc, &srv)))
	{
		return ShaderResourceViewHandle{};
	}
	return srvPool.Create(srv);
}

BufferHandle D3D11Renderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
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
		return BufferHandle{};
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
		return BufferHandle{};
	}
	return bufferPool.Create(buffer);
}

void D3D11Renderer::ReleaseBuffer(BufferHandle& buffer)
{
	if (!buffer.IsValid())
		return;
	ID3D11Buffer* b = bufferPool.Get(buffer);
	SAFERELEASE(b);
	bufferPool.Release(buffer);
}

DXGI_FORMAT D3D11Renderer::FromTextureFormat(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::RGBAFLOAT:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
	case TextureFormat::RGBFLOAT:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;

	case TextureFormat::RGBA32:
		return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::RGBA64:
		return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM;

	case TextureFormat::ALPHA8:
		return DXGI_FORMAT::DXGI_FORMAT_A8_UNORM;

	case TextureFormat::RED8:
		return DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	case TextureFormat::RED16:
		return DXGI_FORMAT::DXGI_FORMAT_R16_UNORM;
	case TextureFormat::REDFLOAT:
		return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	case TextureFormat::RED1:
		return DXGI_FORMAT::DXGI_FORMAT_R1_UNORM;

	case TextureFormat::D32:
		return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	case TextureFormat::RGFLOAT:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	case TextureFormat::RG32:
		return DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM;
	}
	return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
}

bool D3D11Renderer::InitImGui(SDL_Window* window)
{
	if (!ImGui_ImplSDL2_InitForD3D(window))
		return false;
	if (!ImGui_ImplDX11_Init(device, context))
		return false;
	imguiInitialized = true;
	return true;
}

void D3D11Renderer::ImGuiNewFrame(SDL_Window* window)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

void D3D11Renderer::ImGuiRenderDrawData()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
