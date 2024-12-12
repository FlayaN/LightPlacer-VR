#pragma once

#include "ConfigData.h"
#include "LightData.h"
#include "SourceData.h"

class LightManager : public ISingleton<LightManager>
{
public:
	bool ReadConfigs(bool a_reload = false);
	void OnDataLoad();
	void ReloadConfigs();

	std::vector<RE::TESObjectREFRPtr> GetLightAttachedRefs();

	void AddLights(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base, RE::NiAVObject* a_root);
	void ReattachLights(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base);
	void DetachLights(RE::TESObjectREFR* a_ref, bool a_clearData);

	void AddWornLights(RE::TESObjectREFR* a_ref, const RE::BSTSmartPointer<RE::BipedAnim>& a_bipedAnim, std::int32_t a_slot, RE::NiAVObject* a_root);
	void ReattachWornLights(const RE::ActorHandle& a_handle);
	void DetachWornLights(const RE::ActorHandle& a_handle, RE::NiAVObject* a_root);

	void AddTempEffectLights(RE::ReferenceEffect* a_effect, RE::FormID a_effectFormID);
	void ReattachTempEffectLights(RE::ReferenceEffect* a_effect);
	void DetachTempEffectLights(RE::ReferenceEffect* a_effect, bool a_clear);

	void AddLightsToProcessQueue(const RE::TESObjectCELL* a_cell, RE::TESObjectREFR* a_ref);
	void UpdateFlickeringAndConditions(const RE::TESObjectCELL* a_cell);
	void UpdateEmittance(const RE::TESObjectCELL* a_cell);
	void RemoveLightsFromProcessQueue(const RE::TESObjectCELL* a_cell, const RE::ObjectRefHandle& a_handle);

	void UpdateTempEffectLights(RE::ReferenceEffect* a_effect);

	template <class F>
	void ForAllLights(F&& func)
	{
		gameRefLights.read_unsafe([&](auto& map) {
			for (auto& [handle, lightDataVec] : map) {
				for (auto& lightData : lightDataVec) {
					func(lightData);
				}
			}
		});
		gameActorLights.read_unsafe([&](auto& map) {
			for (auto& [handle, nodes] : map) {
				nodes.read_unsafe([&](auto& nodeMap) {
					for (auto& [node, lightDataVec] : nodeMap) {
						for (auto& lightData : lightDataVec) {
							func(lightData);
						}
					}
				});
			}
		});
		gameVisualEffectLights.read_unsafe([&](auto& map) {
			for (auto& [effect, processedLights] : map) {
				for (auto& lightData : processedLights.lights) {
					func(lightData);
				}
			}
		});
	}

	template <class F>
	void ForEachLight(RE::RefHandle a_handle, F&& func)
	{
		gameRefLights.read_unsafe([&](auto& map) {
			if (auto it = map.find(a_handle); it != map.end()) {
				for (auto& lightData : it->second) {
					func(lightData);
				}
			}
		});
	}

	template <class F>
	void ForEachWornLight(RE::RefHandle a_handle, F&& func)
	{
		gameActorLights.read_unsafe([&](auto& map) {
			if (auto it = map.find(a_handle); it != map.end()) {
				it->second.read_unsafe([&](auto& nodeMap) {
					for (auto& [node, lightDataVec] : nodeMap) {
						for (auto& lightData : lightDataVec) {
							func(lightData);
						}
					}
				});
			}
		});
	}

	template <class F>
	void ForEachLight(RE::TESObjectREFR* a_ref, RE::RefHandle a_handle, F&& func)
	{
		if (RE::IsActor(a_ref)) {
			ForEachWornLight(a_handle, func);
		} else {
			ForEachLight(a_handle, func);
		}
	}

private:
	void ProcessConfigs();

	void AttachLightsImpl(const SourceData& a_srcData);
	void AttachConfigLights(const SourceData& a_srcData, const Config::LightSourceData& a_lightData, std::uint32_t a_index);
	void AttachLight(const LightSourceData& a_lightSource, const SourceData& a_srcData, RE::NiNode* a_node, std::uint32_t a_index = 0);
	bool ReattachLightsImpl(const SourceData& a_srcData);

	// members
	std::vector<Config::Format>                         configs;
	StringMap<Config::LightSourceVec>                   gameModels;
	FlatMap<RE::FormID, Config::LightSourceVec>         gameVisualEffects;
	FlatMap<std::uint32_t, Config::AddonLightSourceVec> gameAddonNodes;

	LockedMap<RE::RefHandle, std::vector<REFR_LIGH>>                         gameRefLights;
	LockedMap<RE::RefHandle, LockedMap<std::string, std::vector<REFR_LIGH>>> gameActorLights;         // nodeName
	LockedMap<std::uint32_t, ProcessedEffectLights>                          gameVisualEffectLights;  // effectID
	LockedMap<RE::FormID, MutexGuard<ProcessedREFRLights>>                   processedGameRefLights;

	float flickeringDistanceSq{ 0.0f };
};
