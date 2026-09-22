#pragma once
#include <vector>
#include "math/types.h"
#include "utils/util.h"
#include "namespaces.h"

namespace DUPLEX_NS_GRAPHICS
{
	// A generation-checked handle: index identifies a slot in some backend's HandlePool<T>,
	// generation distinguishes this allocation of that slot from whatever was released and
	// reallocated into it before. Tag makes handles for different resource kinds distinct
	// types, so the compiler rejects e.g. passing a TextureHandle where a BufferHandle is
	// expected.
	template<typename Tag>
	struct Handle
	{
		static constexpr ui32 InvalidIndex = UI32MAX;

		ui32 index = InvalidIndex;
		ui32 generation = 0;

		bool IsValid() const { return index != InvalidIndex; }
		bool operator==(const Handle& other) const { return index == other.index && generation == other.generation; }
		bool operator!=(const Handle& other) const { return !(*this == other); }
	};

	struct BufferTag {};
	struct TextureTag {};
	struct ShaderResourceViewTag {};

	using BufferHandle = Handle<BufferTag>;
	using TextureHandle = Handle<TextureTag>;
	using ShaderResourceViewHandle = Handle<ShaderResourceViewTag>;

	// Owns the backend-native resources (ID3D11Buffer*, VkBuffer, ...) behind stable handles.
	// Each backend keeps its own pool per resource kind (native resource types differ per
	// backend); the index/generation bookkeeping here is shared, backend-agnostic infra.
	template<typename T, typename HandleT>
	class HandlePool
	{
	public:
		HandleT Create(T resource)
		{
			ui32 index;
			if (!freeIndices.empty())
			{
				index = freeIndices.back();
				freeIndices.pop_back();
				slots[index].resource = resource;
				slots[index].alive = true;
			}
			else
			{
				index = static_cast<ui32>(slots.size());
				slots.push_back(Slot{ resource, 0, true });
			}
			return HandleT{ index, slots[index].generation };
		}

		void Release(HandleT& handle)
		{
			if (IsValid(handle))
			{
				Slot& slot = slots[handle.index];
				slot.resource = T{};
				slot.alive = false;
				slot.generation++;
				freeIndices.push_back(handle.index);
			}
			handle = HandleT{};
		}

		bool IsValid(HandleT handle) const
		{
			return handle.IsValid()
				&& handle.index < slots.size()
				&& slots[handle.index].alive
				&& slots[handle.index].generation == handle.generation;
		}

		T Get(HandleT handle) const
		{
			ASSERT_MSG(IsValid(handle), "stale or invalid resource handle");
			return slots[handle.index].resource;
		}

	private:
		struct Slot
		{
			T resource{};
			ui32 generation = 0;
			bool alive = false;
		};
		std::vector<Slot> slots;
		std::vector<ui32> freeIndices;
	};
}
