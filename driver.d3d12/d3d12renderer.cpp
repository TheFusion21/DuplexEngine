// EXTERNAL INCLUDES
#define NOMINMAX
#include <d3dcompiler.h>
#include <cstring>
#include <cstdio>
#include <SDL.h>
#include <SDL_syswm.h>
#include "spirv_cpp.hpp"
#include "spirv_parser.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_reflect.hpp"
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_dx12.h>
// INTERNAL INCLUDES
#include "d3d12renderer.h"
#include "utils/util.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "vertex.h"
#include "log.h"

using namespace DUPLEX_NS_GRAPHICS;
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;
using namespace DUPLEX_NS_LOG;

namespace
{
	ui64 AlignUp(ui64 size, ui64 alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}

	// res/CMakeLists.txt compiles the shared .spirv with -fvk-t-shift 10 0 -fvk-s-shift 20 0, so
	// every SRV's Vulkan binding sits at +10 and every sampler's at +20 - purely to keep them out
	// of the way of cbuffers b0/b1 in a single Vulkan descriptor set (see that file's own
	// comment). SPIRV-Cross's HLSL backend otherwise preserves those binding numbers verbatim as
	// HLSL register numbers, which both overshoots HLSL SM5.0's 16-slot sampler limit (a bound
	// sampler landing on s20 fails to compile outright) and disagrees with where this renderer
	// actually binds resources on the CPU side (t0.. / s0.. - see Render()). This undoes exactly
	// that shift so the cross-compiled HLSL ends up back at the registers the original
	// res/ps/*.hlsl source declared.
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

bool D3D12Renderer::Init(SDL_Window* window, ui32 width, ui32 height)
{
	this->width = width;
	this->height = height;

	if (!CreateDeviceAndSwapChain(window, width, height)) return false;
	if (!CreateRtvAndDsvHeaps()) return false;
	if (!CreateFrameResources()) return false;
	if (!CreateDepthStencil()) return false;
	if (!CreateCommandObjects()) return false;
	if (!CreateFenceObjects()) return false;
	if (!CreateRootSignature()) return false;
	if (!CreateDescriptorHeaps()) return false;
	if (!CreateConstantBuffers()) return false;

	CreateShader();
	if (!pipelineState) return false;

	if (!CreateDefaultTexture()) return false;

	Logger::Renderer().info("D3D12Renderer initialized ({}x{})", width, height);
	return true;
}

bool D3D12Renderer::CreateDeviceAndSwapChain(SDL_Window* window, ui32 width, ui32 height)
{
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(window, &wmInfo))
	{
		MessageBoxA(NULL, "Could not get native window handle from SDL", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	HWND hwnd = wmInfo.info.win.window;

	ui32 factoryFlags = 0;
#ifdef ENGINE_COMPILE_DEBUG
	{
		ID3D12Debug* debugController = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), reinterpret_cast<void**>(&debugController))))
		{
			debugController->EnableDebugLayer();
			SAFERELEASE(debugController);
		}
	}
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	IDXGIFactory4* factory = nullptr;
	if (FAILED(CreateDXGIFactory2(factoryFlags, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(&factory))))
	{
		MessageBoxA(NULL, "Could not create DXGI factory", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	IDXGIAdapter1* adapter = nullptr;
	if (FAILED(factory->EnumAdapters1(0, &adapter)))
	{
		MessageBoxA(NULL, "Could not enumerate adapter", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SAFERELEASE(factory);
		return false;
	}

	DXGI_ADAPTER_DESC1 adapterDesc = {};
	if (SUCCEEDED(adapter->GetDesc1(&adapterDesc)))
	{
		char adapterDescText[128];
		wcstombs(adapterDescText, adapterDesc.Description, 128);
		Logger::Renderer().info("Graphics Device: {}", adapterDescText);
		Logger::Renderer().info("Graphics available Memory: {} MB", static_cast<ui32>(adapterDesc.DedicatedVideoMemory * 9.5367E-7f));
	}

	if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(&device))))
	{
		MessageBoxA(NULL, "Could not create D3D12 device", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SAFERELEASE(adapter);
		SAFERELEASE(factory);
		return false;
	}
	SAFERELEASE(adapter);

#ifdef ENGINE_COMPILE_DEBUG
	{
		ID3D12InfoQueue1* infoQueue = nullptr;
		if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D12InfoQueue1), reinterpret_cast<void**>(&infoQueue))))
		{
			DWORD cookie = 0;
			infoQueue->RegisterMessageCallback(
				[](D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID, LPCSTR pDescription, void*)
				{
					switch (severity)
					{
					case D3D12_MESSAGE_SEVERITY_CORRUPTION:
					case D3D12_MESSAGE_SEVERITY_ERROR:
						Logger::Renderer().error("[D3D12] {}", pDescription);
						break;
					case D3D12_MESSAGE_SEVERITY_WARNING:
						Logger::Renderer().warn("[D3D12] {}", pDescription);
						break;
					case D3D12_MESSAGE_SEVERITY_INFO:
					default:
						Logger::Renderer().info("[D3D12] {}", pDescription);
						break;
					case D3D12_MESSAGE_SEVERITY_MESSAGE:
						Logger::Renderer().trace("[D3D12] {}", pDescription);
						break;
					}
				},
				D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie);
			SAFERELEASE(infoQueue);
		}
	}
#endif

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&commandQueue))))
	{
		MessageBoxA(NULL, "Could not create command queue", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SAFERELEASE(factory);
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = D3D12_FRAME_COUNT;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain1* swapChain1 = nullptr;
	if (FAILED(factory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1)))
	{
		MessageBoxA(NULL, "Could not create swap chain", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SAFERELEASE(factory);
		return false;
	}
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	if (FAILED(swapChain1->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&swapChain))))
	{
		MessageBoxA(NULL, "Could not query IDXGISwapChain3", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SAFERELEASE(swapChain1);
		SAFERELEASE(factory);
		return false;
	}
	SAFERELEASE(swapChain1);
	SAFERELEASE(factory);

	frameIndex = swapChain->GetCurrentBackBufferIndex();
	return true;
}

bool D3D12Renderer::CreateRtvAndDsvHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = D3D12_FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&rtvHeap))))
	{
		MessageBoxA(NULL, "Could not create RTV descriptor heap", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&dsvHeap))))
	{
		MessageBoxA(NULL, "Could not create DSV descriptor heap", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

bool D3D12Renderer::CreateFrameResources()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		if (FAILED(swapChain->GetBuffer(i, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&renderTargets[i]))))
		{
			MessageBoxA(NULL, "Could not get swap chain buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}
	return true;
}

bool D3D12Renderer::CreateDepthStencil()
{
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	depthStencilBuffer = CreateCommittedTexture2D(width, height, DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue);
	if (!depthStencilBuffer)
	{
		MessageBoxA(NULL, "Could not create depth buffer texture", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilBuffer, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
	return true;
}

void D3D12Renderer::ReleaseFrameResources()
{
	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		SAFERELEASE(renderTargets[i]);
	}
	SAFERELEASE(depthStencilBuffer);
}

bool D3D12Renderer::CreateCommandObjects()
{
	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(&commandAllocators[i]))))
		{
			MessageBoxA(NULL, "Could not create command allocator", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0], nullptr, __uuidof(ID3D12GraphicsCommandList), reinterpret_cast<void**>(&commandList))))
	{
		MessageBoxA(NULL, "Could not create command list", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	// BeginScene() always Reset()s against that frame's allocator before recording anything.
	commandList->Close();

	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(&uploadCommandAllocator))))
	{
		MessageBoxA(NULL, "Could not create upload command allocator", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadCommandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), reinterpret_cast<void**>(&uploadCommandList))))
	{
		MessageBoxA(NULL, "Could not create upload command list", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	uploadCommandList->Close();

	return true;
}

bool D3D12Renderer::CreateFenceObjects()
{
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), reinterpret_cast<void**>(&fence))))
	{
		MessageBoxA(NULL, "Could not create fence", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	fenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent)
	{
		MessageBoxA(NULL, "Could not create fence event", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

bool D3D12Renderer::CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE srvRange = {};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = SRVS_PER_DRAW;
	srvRange.BaseShaderRegister = 0;
	srvRange.RegisterSpace = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// b0 (modelConstant) / b1 (worldConstant) as root CBVs - bound directly by GPU virtual
	// address per draw (see Render()), no descriptor heap entry needed for either. t0-t5 (the six
	// material textures - see meshsystem.h/bsdfPixel.hlsl) as a descriptor table, since those
	// vary in count/identity per material and do need a real heap slice.
	D3D12_ROOT_PARAMETER params[3] = {};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.ShaderRegister = 0;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[1].Descriptor.ShaderRegister = 1;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[2].DescriptorTable.NumDescriptorRanges = 1;
	params[2].DescriptorTable.pDescriptorRanges = &srvRange;
	params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Matches D3D11Renderer's defaultSampler/computeSampler descs exactly. Static samplers avoid
	// needing a sampler descriptor heap at all (unlike VulkanRenderer, which has to allocate a
	// real sampler descriptor per draw - see its Render()), since neither sampler ever changes.
	D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
	samplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
	samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[0].MaxAnisotropy = 16; // D3D12_REQ_MAXANISOTROPY
	samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplers[0].MinLOD = 0.0f;
	samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[0].ShaderRegister = 0;
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[1].MaxAnisotropy = 1;
	samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplers[1].MinLOD = 0.0f;
	samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[1].ShaderRegister = 1;
	samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = 3;
	rootDesc.pParameters = params;
	rootDesc.NumStaticSamplers = 2;
	rootDesc.pStaticSamplers = samplers;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob)))
	{
		if (errorBlob)
		{
			MessageBoxA(NULL, static_cast<char*>(errorBlob->GetBufferPointer()), "ERROR", MB_OK | MB_ICONEXCLAMATION);
			SAFERELEASE(errorBlob);
		}
		return false;
	}

	bool ok = SUCCEEDED(device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature)));
	SAFERELEASE(signatureBlob);
	SAFERELEASE(errorBlob);
	if (!ok)
	{
		MessageBoxA(NULL, "Could not create root signature", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

bool D3D12Renderer::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = D3D12_FRAME_COUNT * MAX_DRAWS_PER_FRAME * SRVS_PER_DRAW;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&srvHeap))))
	{
		MessageBoxA(NULL, "Could not create SRV descriptor heap", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC cpuHeapDesc = {};
	cpuHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cpuHeapDesc.NumDescriptors = CpuSrvHeapCapacity;
	cpuHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(device->CreateDescriptorHeap(&cpuHeapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&cpuSrvHeap))))
	{
		MessageBoxA(NULL, "Could not create CPU SRV descriptor heap", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	cpuSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}

ID3D12Resource* D3D12Renderer::CreateUploadResource(ui64 size, const void* data)
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* resource = nullptr;
	if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource))))
	{
		return nullptr;
	}

	if (data != nullptr)
	{
		D3D12_RANGE noRead = { 0, 0 };
		ui8* mapped = nullptr;
		if (SUCCEEDED(resource->Map(0, &noRead, reinterpret_cast<void**>(&mapped))))
		{
			memcpy(mapped, data, static_cast<size_t>(size));
			resource->Unmap(0, nullptr);
		}
	}

	return resource;
}

bool D3D12Renderer::CreateConstantBuffers()
{
	ui64 modelSlotStride = AlignUp(sizeof(modelConstant), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	ui64 worldSlotStride = AlignUp(sizeof(worldConstant), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	D3D12_RANGE noRead = { 0, 0 };

	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		ID3D12Resource* modelResource = CreateUploadResource(modelSlotStride * MAX_DRAWS_PER_FRAME, nullptr);
		if (!modelResource)
		{
			MessageBoxA(NULL, "Could not create model constant buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		modelBuffers[i].resource = modelResource;
		modelBuffers[i].slotStride = static_cast<ui32>(modelSlotStride);
		modelBuffers[i].gpuAddress = modelResource->GetGPUVirtualAddress();
		if (FAILED(modelResource->Map(0, &noRead, reinterpret_cast<void**>(&modelBuffers[i].mapped))))
		{
			MessageBoxA(NULL, "Could not map model constant buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		ID3D12Resource* worldResource = CreateUploadResource(worldSlotStride, nullptr);
		if (!worldResource)
		{
			MessageBoxA(NULL, "Could not create world constant buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		worldBuffers[i].resource = worldResource;
		worldBuffers[i].slotStride = static_cast<ui32>(worldSlotStride);
		worldBuffers[i].gpuAddress = worldResource->GetGPUVirtualAddress();
		if (FAILED(worldResource->Map(0, &noRead, reinterpret_cast<void**>(&worldBuffers[i].mapped))))
		{
			MessageBoxA(NULL, "Could not map world constant buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
	return true;
}

bool D3D12Renderer::CreateDefaultTexture()
{
	ui32 white = 0xFFFFFFFFu;
	defaultTextureHandle = CreateTexture(1, 1, 1, TextureFormat::RGBA32, &white);
	if (!defaultTextureHandle.IsValid())
		return false;
	defaultTextureView = CreateTextureSRV(defaultTextureHandle, TextureFormat::RGBA32);
	return defaultTextureView.IsValid();
}

void D3D12Renderer::CreateShader()
{
	ID3DBlob* vertexShaderBlob = nullptr;
	ID3DBlob* pixelShaderBlob = nullptr;

	{
		FILE* file = fopen("./bin/data/shd/bsdfVertex.spirv", "rb");
		if (file == nullptr)
		{
			// Was an uncaught throw std::string(...) - nothing catches exceptions this deep in
			// Init(), so it crashed silently instead of explaining itself. Matches D3D11Renderer's
			// identical fix: this is the exact failure a wrong working directory produces (e.g.
			// double-clicking the exe in Explorer, which sets CWD to its own folder, not the repo
			// root these ./bin/... paths assume).
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
		RemapVulkanShiftedBindingsToHlsl(hlslCompiler);
		std::string vertexShaderHLSLSource = hlslCompiler.compile();

		ID3DBlob* errorBlob = nullptr;
		if (FAILED(D3DCompile(vertexShaderHLSLSource.c_str(), vertexShaderHLSLSource.length(), nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vertexShaderBlob, &errorBlob)))
		{
			if (errorBlob)
			{
				MessageBoxA(NULL, static_cast<char*>(errorBlob->GetBufferPointer()), "ERROR", MB_OK | MB_ICONEXCLAMATION);
				SAFERELEASE(errorBlob);
			}
			return;
		}
		SAFERELEASE(errorBlob);
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

		ID3DBlob* errorBlob = nullptr;
		if (FAILED(D3DCompile(pixelShaderHLSLSource.c_str(), pixelShaderHLSLSource.length(), nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pixelShaderBlob, &errorBlob)))
		{
			if (errorBlob)
			{
				MessageBoxA(NULL, static_cast<char*>(errorBlob->GetBufferPointer()), "ERROR", MB_OK | MB_ICONEXCLAMATION);
				SAFERELEASE(errorBlob);
			}
			SAFERELEASE(vertexShaderBlob);
			return;
		}
		SAFERELEASE(errorBlob);
	}

	// Matches D3D11Renderer::CreateShader's vertexElemDesc exactly (see its comment on why
	// BITTANGENT0 stays even though the compiled shader doesn't read it - DXC strips unused
	// vertex inputs, and an input layout may legally declare more elements than the shader
	// actually consumes).
	D3D12_INPUT_ELEMENT_DESC vertexElemDesc[5] = {};
	vertexElemDesc[0].SemanticName = "POSITION";
	vertexElemDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[0].AlignedByteOffset = 0;
	vertexElemDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	vertexElemDesc[1].SemanticName = "NORMAL";
	vertexElemDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	vertexElemDesc[2].SemanticName = "TEXCOORD";
	vertexElemDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexElemDesc[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	vertexElemDesc[3].SemanticName = "TANGENT";
	vertexElemDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	vertexElemDesc[4].SemanticName = "BITTANGENT";
	vertexElemDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElemDesc[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	vertexElemDesc[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout.pInputElementDescs = vertexElemDesc;
	psoDesc.InputLayout.NumElements = 5;
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS.pShaderBytecode = vertexShaderBlob->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vertexShaderBlob->GetBufferSize();
	psoDesc.PS.pShaderBytecode = pixelShaderBlob->GetBufferPointer();
	psoDesc.PS.BytecodeLength = pixelShaderBlob->GetBufferSize();

	psoDesc.RasterizerState.FillMode = wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = wireframe ? D3D12_CULL_MODE_NONE : D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = wireframe ? FALSE : TRUE;
	psoDesc.RasterizerState.AntialiasedLineEnable = wireframe;

	// Matches D3D11Renderer's blendDesc (SrcBlend=SRC_ALPHA, DestBlend=INV_SRC_ALPHA,
	// BlendOp=ADD; SrcBlendAlpha=ONE, DestBlendAlpha=ZERO).
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipelineState))))
	{
		MessageBoxA(NULL, "Could not create pipeline state object", "ERROR", MB_OK | MB_ICONEXCLAMATION);
	}

	SAFERELEASE(vertexShaderBlob);
	SAFERELEASE(pixelShaderBlob);
}

void D3D12Renderer::SetViewPort()
{
	// Like VulkanRenderer, D3D12 has no persistent per-device viewport state - it's recorded
	// into the command list fresh every frame in BeginScene() instead, using this->width/height
	// (which Init()/Resize() already keep current).
}

void D3D12Renderer::SetActiveCamera(Vec3 eye, Mat4x4 viewProj)
{
	worldLocalBuffer.eye = eye;
	worldLocalBuffer.projView = ToShaderLayout(viewProj);
}

void D3D12Renderer::ClearLights()
{
	lights.clear();
}

void D3D12Renderer::SetLight(GpuLight lightDescriptor)
{
	lightDescriptor.transform = ToShaderLayout(lightDescriptor.transform);
	lights.push_back(lightDescriptor);
}

void D3D12Renderer::BeginScene()
{
	if (fence->GetCompletedValue() < fenceValues[frameIndex])
	{
		fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	commandAllocators[frameIndex]->Reset();
	commandList->Reset(commandAllocators[frameIndex], pipelineState);

	commandList->SetGraphicsRootSignature(rootSignature);
	ID3D12DescriptorHeap* heaps[] = { srvHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &viewport);
	D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	commandList->RSSetScissorRects(1, &scissor);

	D3D12_RESOURCE_BARRIER toRenderTarget = {};
	toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toRenderTarget.Transition.pResource = renderTargets[frameIndex];
	toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &toRenderTarget);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<SIZE_T>(frameIndex) * rtvDescriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Once per frame (matching D3D11Renderer::BeginScene's worldBuffer map/update) - camera +
	// lights, read by every draw this frame via worldBuffers[frameIndex].
	worldLocalBuffer.lightCount = static_cast<ui32>(lights.size());
	for (ui32 i = 0; i < lights.size() && i < MAX_LIGHTS; i++)
	{
		worldLocalBuffer.lights[i] = lights[i];
	}
	memcpy(worldBuffers[frameIndex].mapped, &worldLocalBuffer, sizeof(worldConstant));

	drawIndexThisFrame = 0;
}

void D3D12Renderer::Render(Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount)
{
	ASSERT_MSG(drawIndexThisFrame < MAX_DRAWS_PER_FRAME, "exceeded MAX_DRAWS_PER_FRAME - raise the constant in d3d12renderer.h");

	modelConstant modelCB;
	modelCB.world = ToShaderLayout(transformMat);
	ui8* modelDst = modelBuffers[frameIndex].mapped + static_cast<ui64>(drawIndexThisFrame) * modelBuffers[frameIndex].slotStride;
	memcpy(modelDst, &modelCB, sizeof(modelConstant));
	D3D12_GPU_VIRTUAL_ADDRESS modelGpuAddr = modelBuffers[frameIndex].gpuAddress + static_cast<ui64>(drawIndexThisFrame) * modelBuffers[frameIndex].slotStride;

	commandList->SetGraphicsRootConstantBufferView(0, modelGpuAddr);
	commandList->SetGraphicsRootConstantBufferView(1, worldBuffers[frameIndex].gpuAddress);

	// Engine texture slot -> shader register(tN): slots 0-5 map straight to t0-t5 (unlike
	// VulkanRenderer, which shifts to 10/11/13/14/15 - see res/CMakeLists.txt's comment for why
	// that shift only applies to the SPIR-V/Vulkan path, not this cross-compiled-HLSL one).
	ui32 baseSlot = (frameIndex * MAX_DRAWS_PER_FRAME + drawIndexThisFrame) * SRVS_PER_DRAW;
	D3D12_CPU_DESCRIPTOR_HANDLE destCpuStart = srvHeap->GetCPUDescriptorHandleForHeapStart();
	for (ui32 i = 0; i < SRVS_PER_DRAW; i++)
	{
		ShaderResourceViewHandle view = (i < pendingTextureViews.size()) ? pendingTextureViews[i] : ShaderResourceViewHandle{};
		D3D12_CPU_DESCRIPTOR_HANDLE source = view.IsValid() ? srvPool.Get(view).cpuHandle : srvPool.Get(defaultTextureView).cpuHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE dest = destCpuStart;
		dest.ptr += static_cast<SIZE_T>(baseSlot + i) * srvDescriptorSize;
		device->CopyDescriptorsSimple(1, dest, source, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE tableHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	tableHandle.ptr += static_cast<UINT64>(baseSlot) * srvDescriptorSize;
	commandList->SetGraphicsRootDescriptorTable(2, tableHandle);

	D3D12Buffer vb = bufferPool.Get(vertexBuffer);
	D3D12Buffer ib = bufferPool.Get(indexBuffer);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = vb.resource->GetGPUVirtualAddress();
	vbv.SizeInBytes = vb.size;
	vbv.StrideInBytes = sizeof(Vertex);
	commandList->IASetVertexBuffers(0, 1, &vbv);

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = ib.resource->GetGPUVirtualAddress();
	ibv.SizeInBytes = ib.size;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	commandList->IASetIndexBuffer(&ibv);

	commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

	drawIndexThisFrame++;
	pendingTextureViews.clear();
}

void D3D12Renderer::EndScene()
{
	D3D12_RESOURCE_BARRIER toPresent = {};
	toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toPresent.Transition.pResource = renderTargets[frameIndex];
	toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &toPresent);

	commandList->Close();
	ID3D12CommandList* lists[] = { commandList };
	commandQueue->ExecuteCommandLists(1, lists);

	swapChain->Present(vsyncEnable ? 1 : 0, 0);

	MoveToNextFrame();
}

void D3D12Renderer::MoveToNextFrame()
{
	const ui64 signalValue = nextFenceValue;
	commandQueue->Signal(fence, signalValue);
	nextFenceValue++;
	fenceValues[frameIndex] = signalValue;

	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void D3D12Renderer::WaitForGpu()
{
	const ui64 signalValue = nextFenceValue;
	commandQueue->Signal(fence, signalValue);
	nextFenceValue++;
	if (fence->GetCompletedValue() < signalValue)
	{
		fence->SetEventOnCompletion(signalValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	// Every frame's command allocator is now safe to Reset() - keeps the per-frame fence values
	// consistent with what just got waited on, so a subsequent BeginScene() doesn't wait again.
	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		fenceValues[i] = signalValue;
	}
}

void D3D12Renderer::Shutdown()
{
	Logger::Renderer().info("D3D12Renderer shutting down");

	if (device)
	{
		WaitForGpu();
	}

	if (imguiInitialized)
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		SAFERELEASE(imguiSrvHeap);
		imguiInitialized = false;
	}

	ReleaseTexture(defaultTextureHandle);
	ReleaseTextureSRV(defaultTextureView);

	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		SAFERELEASE(modelBuffers[i].resource);
		SAFERELEASE(worldBuffers[i].resource);
	}

	if (swapChain)
	{
		swapChain->SetFullscreenState(FALSE, nullptr);
	}

	ReleaseFrameResources();
	SAFERELEASE(rtvHeap);
	SAFERELEASE(dsvHeap);
	SAFERELEASE(srvHeap);
	SAFERELEASE(cpuSrvHeap);
	SAFERELEASE(rootSignature);
	SAFERELEASE(pipelineState);

	for (ui32 i = 0; i < D3D12_FRAME_COUNT; i++)
	{
		SAFERELEASE(commandAllocators[i]);
	}
	SAFERELEASE(commandList);
	SAFERELEASE(uploadCommandAllocator);
	SAFERELEASE(uploadCommandList);

	if (fenceEvent)
	{
		CloseHandle(fenceEvent);
		fenceEvent = nullptr;
	}
	SAFERELEASE(fence);

	SAFERELEASE(commandQueue);
	SAFERELEASE(swapChain);
	SAFERELEASE(device);
}

DXGI_FORMAT D3D12Renderer::FromTextureFormat(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::RGBAFLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case TextureFormat::RGBFLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;

	case TextureFormat::RGBA32:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::RGBA64:
		return DXGI_FORMAT_R16G16B16A16_UNORM;

	case TextureFormat::ALPHA8:
		return DXGI_FORMAT_A8_UNORM;

	case TextureFormat::RED8:
		return DXGI_FORMAT_R8_UNORM;
	case TextureFormat::RED16:
		return DXGI_FORMAT_R16_UNORM;
	case TextureFormat::REDFLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case TextureFormat::RED1:
		return DXGI_FORMAT_R1_UNORM;

	// Pure 32-bit depth, no stencil - matches VulkanRenderer's choice (VK_FORMAT_D32_SFLOAT)
	// rather than D3D11's DXGI_FORMAT_D32_FLOAT_S8X24_UINT, since this backend's depth buffer is
	// created through its own dedicated CreateDepthStencil() (mirroring Vulkan's
	// CreateDepthResources()), not through this generic path.
	case TextureFormat::D32:
		return DXGI_FORMAT_D32_FLOAT;

	case TextureFormat::RGFLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
	case TextureFormat::RG32:
		return DXGI_FORMAT_R16G16_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}

ID3D12Resource* D3D12Renderer::CreateCommittedTexture2D(ui32 texWidth, ui32 texHeight, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = texWidth;
	desc.Height = texHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	ID3D12Resource* resource = nullptr;
	if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, initialState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource))))
	{
		return nullptr;
	}
	return resource;
}

ID3D12Resource* D3D12Renderer::UploadTextureData(ID3D12Resource* texture, ui32 texWidth, ui32 texHeight, DXGI_FORMAT format, ui32 pixelSize, const void* data)
{
	ui32 rowPitch = static_cast<ui32>(AlignUp(static_cast<ui64>(texWidth) * pixelSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	ui64 uploadSize = static_cast<ui64>(rowPitch) * texHeight;

	ID3D12Resource* uploadBuffer = CreateUploadResource(uploadSize, nullptr);
	if (!uploadBuffer)
	{
		MessageBoxA(NULL, "Could not create texture upload buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return nullptr;
	}

	ui8* mapped = nullptr;
	D3D12_RANGE noRead = { 0, 0 };
	if (SUCCEEDED(uploadBuffer->Map(0, &noRead, reinterpret_cast<void**>(&mapped))))
	{
		const ui8* src = static_cast<const ui8*>(data);
		for (ui32 y = 0; y < texHeight; y++)
		{
			memcpy(mapped + static_cast<ui64>(y) * rowPitch, src + static_cast<ui64>(y) * texWidth * pixelSize, static_cast<size_t>(texWidth) * pixelSize);
		}
		uploadBuffer->Unmap(0, nullptr);
	}

	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = texture;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
	srcLoc.pResource = uploadBuffer;
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLoc.PlacedFootprint.Offset = 0;
	srcLoc.PlacedFootprint.Footprint.Format = format;
	srcLoc.PlacedFootprint.Footprint.Width = texWidth;
	srcLoc.PlacedFootprint.Footprint.Height = texHeight;
	srcLoc.PlacedFootprint.Footprint.Depth = 1;
	srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

	uploadCommandList->CopyTextureRegion(&dst, 0, 0, 0, &srcLoc, nullptr);

	return uploadBuffer;
}

TextureHandle D3D12Renderer::CreateTexture(ui32 texWidth, ui32 texHeight, ui32 levels, TextureFormat format, void* data)
{
	DXGI_FORMAT dxgiFormat = FromTextureFormat(format);
	bool isDepth = (format == TextureFormat::D32);

	D3D12_RESOURCE_FLAGS flags = isDepth ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES initialState = isDepth
		? D3D12_RESOURCE_STATE_DEPTH_WRITE
		: (data != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_CLEAR_VALUE clearValue = {};
	D3D12_CLEAR_VALUE* clearValuePtr = nullptr;
	if (isDepth)
	{
		clearValue.Format = dxgiFormat;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		clearValuePtr = &clearValue;
	}

	ID3D12Resource* resource = CreateCommittedTexture2D(texWidth, texHeight, dxgiFormat, flags, initialState, clearValuePtr);
	if (!resource)
	{
		MessageBoxA(NULL, "Could not create texture2D", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return TextureHandle{};
	}

	if (data != nullptr && !isDepth)
	{
		ui32 pixelSize = PixelSizeFromTextureFormat(format);

		uploadCommandAllocator->Reset();
		uploadCommandList->Reset(uploadCommandAllocator, nullptr);

		ID3D12Resource* stagingBuffer = UploadTextureData(resource, texWidth, texHeight, dxgiFormat, pixelSize, data);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		uploadCommandList->ResourceBarrier(1, &barrier);

		uploadCommandList->Close();
		ID3D12CommandList* lists[] = { uploadCommandList };
		commandQueue->ExecuteCommandLists(1, lists);
		// Blocks until the copy recorded above has actually executed on the GPU - only then is
		// the staging buffer (which the copy reads from) safe to release.
		WaitForGpu();
		SAFERELEASE(stagingBuffer);
	}

	D3D12Texture tex;
	tex.resource = resource;
	tex.format = dxgiFormat;
	return texturePool.Create(tex);
}

void D3D12Renderer::ReleaseTexture(TextureHandle& texture)
{
	if (!texture.IsValid())
		return;
	D3D12Texture tex = texturePool.Get(texture);
	SAFERELEASE(tex.resource);
	texturePool.Release(texture);
}

void D3D12Renderer::UseTexture(ui32 slot, ShaderResourceViewHandle view)
{
	while (slot >= pendingTextureViews.size())
	{
		pendingTextureViews.push_back(ShaderResourceViewHandle{});
	}
	pendingTextureViews[slot] = view;
}

ShaderResourceViewHandle D3D12Renderer::CreateTextureSRV(TextureHandle texture, TextureFormat format)
{
	ASSERT_MSG(cpuSrvHeapNextSlot < CpuSrvHeapCapacity, "exceeded CpuSrvHeapCapacity - raise it in d3d12renderer.h");

	D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuSrvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(cpuSrvHeapNextSlot) * cpuSrvDescriptorSize;
	cpuSrvHeapNextSlot++;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = FromTextureFormat(format);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	ID3D12Resource* resource = texturePool.Get(texture).resource;
	device->CreateShaderResourceView(resource, &srvDesc, handle);

	// See D3D12ShaderResourceView's declaration - this SRV needs its own reference to the
	// resource independent of the TextureHandle's, since a D3D12 descriptor doesn't hold one
	// automatically the way D3D11's SRVs do.
	resource->AddRef();

	D3D12ShaderResourceView view;
	view.cpuHandle = handle;
	view.resource = resource;
	return srvPool.Create(view);
}

void D3D12Renderer::ReleaseTextureSRV(ShaderResourceViewHandle& srv)
{
	if (!srv.IsValid())
		return;
	// The underlying cpuSrvHeap slot is intentionally not reclaimed - see its declaration in
	// d3d12renderer.h. The resource reference this SRV took in CreateTextureSRV/CreateCubemapSRV
	// does need releasing here, though - see D3D12ShaderResourceView's declaration.
	D3D12ShaderResourceView view = srvPool.Get(srv);
	SAFERELEASE(view.resource);
	srvPool.Release(srv);
}

ShaderResourceViewHandle D3D12Renderer::CreateCubemapSRV(TextureHandle cubemap, TextureFormat format)
{
	// Matches D3D11Renderer's CreateCubemapSRV, including the same latent gap noted in
	// VulkanRenderer's own version: nothing anywhere in the engine actually creates a real
	// 6-layer cubemap resource (CreateTexture always creates DepthOrArraySize=1), so this is
	// unexercised parity code on all three backends today, not something newly broken here.
	ASSERT_MSG(cpuSrvHeapNextSlot < CpuSrvHeapCapacity, "exceeded CpuSrvHeapCapacity - raise it in d3d12renderer.h");

	D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuSrvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(cpuSrvHeapNextSlot) * cpuSrvDescriptorSize;
	cpuSrvHeapNextSlot++;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = FromTextureFormat(format);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.TextureCube.MipLevels = 1;

	ID3D12Resource* cubemapResource = texturePool.Get(cubemap).resource;
	device->CreateShaderResourceView(cubemapResource, &srvDesc, handle);
	cubemapResource->AddRef();

	D3D12ShaderResourceView view;
	view.cpuHandle = handle;
	view.resource = cubemapResource;
	return srvPool.Create(view);
}

BufferHandle D3D12Renderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
	if (type != BufferType::Vertex && type != BufferType::Index && type != BufferType::Constant)
	{
		MessageBoxA(NULL, "invalid buffer type", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return BufferHandle{};
	}

	// Every buffer this engine creates today (vertex/index buffers at import time, constant
	// buffers) is small and either write-once or CPU-updated - creating everything directly in
	// the upload heap (persistently CPU-writable) is simpler than a default-heap+staging-copy
	// path and, at this engine's current content scale, costs nothing that matters. usage is
	// unused as a result: both Default and Dynamic end up with the same CPU-writable resource.
	(void)usage;
	ID3D12Resource* resource = CreateUploadResource(static_cast<ui64>(dataSize), data);
	if (!resource)
	{
		MessageBoxA(NULL, "Could not create buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return BufferHandle{};
	}

	D3D12Buffer buf;
	buf.resource = resource;
	buf.size = static_cast<ui32>(dataSize);
	return bufferPool.Create(buf);
}

void D3D12Renderer::ReleaseBuffer(BufferHandle& buffer)
{
	if (!buffer.IsValid())
		return;
	D3D12Buffer buf = bufferPool.Get(buffer);
	SAFERELEASE(buf.resource);
	bufferPool.Release(buffer);
}

bool D3D12Renderer::Resize(ui32 newWidth, ui32 newHeight)
{
	if (newWidth == 0 || newHeight == 0)
		return false; // minimized

	WaitForGpu();

	ReleaseFrameResources();

	this->width = newWidth;
	this->height = newHeight;

	if (FAILED(swapChain->ResizeBuffers(D3D12_FRAME_COUNT, newWidth, newHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)))
	{
		MessageBoxA(NULL, "Could not resize swap chain", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	if (!CreateFrameResources())
		return false;
	if (!CreateDepthStencil())
		return false;

	SetViewPort();
	return true;
}

bool D3D12Renderer::InitImGui(SDL_Window* window)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&imguiSrvHeap))))
		return false;

	if (!ImGui_ImplSDL2_InitForD3D(window))
		return false;

	if (!ImGui_ImplDX12_Init(device, D3D12_FRAME_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM, imguiSrvHeap,
		imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(), imguiSrvHeap->GetGPUDescriptorHandleForHeapStart()))
		return false;

	imguiInitialized = true;
	return true;
}

void D3D12Renderer::ImGuiNewFrame(SDL_Window* window)
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

void D3D12Renderer::ImGuiRenderDrawData()
{
	ImGui::Render();
	// Switches the command list's active descriptor heap to ImGui's own (see InitImGui) -
	// everything this renderer's own Render() calls needed from srvHeap was already recorded
	// earlier in this same command list, before this call (see editor.cpp's Run() loop).
	ID3D12DescriptorHeap* heaps[] = { imguiSrvHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
