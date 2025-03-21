#include "Detach.h"

namespace Hooks::Detach
{
	// clears light from the shadowscenenode
	struct RemoveLight
	{
		static void thunk(RE::TESObjectREFR* a_this, bool a_isMagicLight)
		{
			LightManager::GetSingleton()->DetachLights(a_this, false);

			func(a_this, a_isMagicLight);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(19253, 19679) };
			stl::hook_function_prologue<RemoveLight, 8>(target.address());

			logger::info("Hooked TESObjectREFR::RemoveLight");
		}
	};

	// clears light from the shadowscenenode + nilight ptr
	struct GetLightData
	{
		static RE::REFR_LIGHT* thunk(RE::ExtraDataList* a_list)
		{
			if (auto* ref = stl::adjust_pointer<RE::TESObjectREFR>(a_list, -0x70)) {
				LightManager::GetSingleton()->DetachLights(ref, true);
			}

			return func(a_list);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(19102, 19504), OFFSET(0xC0, 0xCA)),    // TESObjectREFR::ClearData
				std::make_pair(RELOCATION_ID(19302, 19729), OFFSET(0x63C, 0x63A)),  // TESObjectREFR::Set3D
			};

			for (const auto& [address, offset] : targets) {
				REL::Relocation<std::uintptr_t> target{ address, offset };
				stl::write_thunk_call<GetLightData>(target.address());
			}

			logger::info("Hooked ExtraDataList::GetLightData");
		}
	};

	struct RunBiped3DDetach
	{
		static void thunk(const RE::ActorHandle& a_handle, RE::NiAVObject* a_node)
		{
			LightManager::GetSingleton()->DetachWornLights(a_handle, a_node);

			func(a_handle, a_node);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(15495, 15660) };
			stl::hook_function_prologue<RunBiped3DDetach, 5>(target.address());

			logger::info("Hooked BipedAnim::RunBiped3DDetach");
		}
	};

	// casting art
	struct BGSAttachTechniques__DetachItem
	{
		static bool thunk(RE::RefAttachTechniqueInput& a_this)
		{
			LightManager::GetSingleton()->DetachCastingLights(a_this);

			return func(a_this);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(33371, 34152), OFFSET(0x26, 0xA7)),  // ActorMagicCaster::DetachCastingArt
#ifndef SKYRIM_AE
				std::make_pair(RELOCATION_ID(33375, 0), OFFSET(0x52, 0)),
#endif
			};

			for (const auto& [address, offset] : targets) {
				REL::Relocation<std::uintptr_t> target{ address, offset };
				stl::write_thunk_call<BGSAttachTechniques__DetachItem>(target.address());
			}
		}
	};

	struct Suspend
	{
		static void thunk(RE::ShaderReferenceEffect* a_this)
		{
			bool suspended = a_this->flags.any(RE::ShaderReferenceEffect::Flag::kSuspended);
			func(a_this);
			if (suspended != a_this->flags.any(RE::ShaderReferenceEffect::Flag::kSuspended)) {
				LightManager::GetSingleton()->DetachTempEffectLights(a_this, false);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static constexpr std::size_t                   idx{ 0x37 };

		static void Install()
		{
			stl::write_vfunc<RE::ShaderReferenceEffect, Suspend>();
			logger::info("Hooked ShaderReferenceEffect::Suspend"sv);
		}
	};

	void Install()
	{
		RemoveLight::Install();
		GetLightData::Install();
		RunBiped3DDetach::Install();
		BGSAttachTechniques__DetachItem::Install();
		BSTempEffect::DetachImpl<RE::ShaderReferenceEffect>::Install();
		BSTempEffect::DetachImpl<RE::ModelReferenceEffect>::Install();
		Suspend::Install();
	}
}
