#pragma once

#include "LightControllers.h"

struct SourceData;

struct LightData
{
	// CS light flags
	enum class Flags
	{
		None = 0,
		PortalStrict = (1 << 0),
		Shadow = (1 << 1),
		Simple = (1 << 2),

		UpdateOnWaiting = (1 << 25),
		UpdateOnCellTransition = (1 << 26),
		NeedsUpdate = UpdateOnWaiting | UpdateOnCellTransition,

		SyncAddonNodes = (1 << 27),
		IgnoreScale = (1 << 28),
		RandomAnimStart = (1 << 29),
		NoExternalEmittance = (1 << 30)
	};

	enum class CullFlags : std::uint8_t
	{
		None = 0,
		Hidden = (1 << 0),
		Culled = (1 << 1)
	};

	bool                                     GetCastsShadows() const;
	RE::NiColor                              GetDiffuse() const;
	float                                    GetRadius() const;
	float                                    GetFade() const;
	float                                    GetScaledRadius(float a_radius, float a_scale) const;
	float                                    GetScaledFade(float a_fade, float a_scale) const;
	float                                    GetScaledRadius(float a_scale) const;
	float                                    GetScaledFade(float a_scale) const;
	float                                    GetFOV() const;
	float                                    GetFalloff() const;
	float                                    GetNearDistance() const;
	static std::string                       GetLightName(const SourceData& a_srcData, std::string_view a_lightEDID, std::uint32_t a_index);
	static std::string                       GetNodeName(const RE::NiPoint3& a_point, std::uint32_t a_index);
	static std::string                       GetNodeName(RE::NiAVObject* a_obj, std::uint32_t a_index);
	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS GetParams(RE::TESObjectREFR* a_ref) const;
	bool                                     GetPortalStrict() const;
	bool                                     IsDynamicLight(RE::TESObjectREFR* a_ref) const;
	bool                                     IsValid() const;

	std::tuple<RE::BSLight*, RE::NiPointLight*, RE::NiAVObject*> GenLight(RE::TESObjectREFR* a_ref, RE::NiNode* a_node, std::string_view a_lightName, float a_scale) const;  // [bsLight, niLight, debugMarker]

	// members
	RE::TESObjectLIGH*                 light{ nullptr };
	RE::NiColor                        color{ RE::COLOR_BLACK };
	float                              radius{ 0.0f };
	float                              fade{ 0.0f };
	float                              fov{ 0.0f };
	float                              shadowDepthBias{ 1.0f };
	RE::NiPoint3                       offset;
	RE::NiMatrix3                      rotation;
	REX::EnumSet<Flags, std::uint32_t> flags{ Flags::None };
	RE::TESForm*                       emittanceForm{ nullptr };
	std::shared_ptr<RE::TESCondition>  conditions;
	StringSet                          conditionalNodes;

	constexpr static auto LP_LIGHT = "LP_Light"sv;
	constexpr static auto LP_NODE = "LP_Node"sv;
	constexpr static auto LP_DEBUG = "LP_DebugMarker"sv;

private:
	struct MARKER_CREATE_PARAMS
	{
		const char* modelName;
		const char* shapeName;
		float       scale;
		bool        flipModel;
	};

	static std::string   GetDebugMarkerName(std::string_view a_lightName);
	RE::NiAVObject*      AttachDebugMarker(RE::NiNode* a_node, std::string_view a_debugMarkerName) const;
	static void          PostProcessDebugMarker(RE::NiAVObject* a_obj, const MARKER_CREATE_PARAMS& a_params, std::string_view a_debugMarkerName);
	MARKER_CREATE_PARAMS GetDebugMarkerParams() const;
};

struct LightSourceData
{
	LightSourceData() = default;

	void ReadFlags();
	void ReadConditions();
	bool PostProcess();

	bool IsStaticLight() const;

	RE::NiNode* GetOrCreateNode(RE::NiNode* a_root, const RE::NiPoint3& a_point, std::uint32_t a_index) const;
	RE::NiNode* GetOrCreateNode(RE::NiNode* a_root, const std::string& a_nodeName, std::uint32_t a_index) const;
	RE::NiNode* GetOrCreateNode(RE::NiNode* a_root, RE::NiAVObject* a_obj, std::uint32_t a_index) const;

	// members
	LightData                data;
	std::string              lightEDID;
	std::string              emittanceFormEDID;
	std::string              flags;
	std::vector<std::string> conditions;
	ColorKeyframeSequence    colorController;
	FloatKeyframeSequence    radiusController;
	FloatKeyframeSequence    fadeController;
	PositionKeyframeSequence positionController;
	RotationKeyframeSequence rotationController;
	AIOKeyframeSequence      lightController;
};

template <>
struct glz::meta<LightSourceData>
{
	using T = LightSourceData;
	static constexpr auto value = object(
		"light", &T::lightEDID,
		"color", [](auto&& self) -> auto& { return self.data.color; },
		"radius", [](auto&& self) -> auto& { return self.data.radius; },
		"fade", [](auto&& self) -> auto& { return self.data.fade; },
		"fov", [](auto&& self) -> auto& { return self.data.fov; },
		"shadowDepthBias", [](auto&& self) -> auto& { return self.data.shadowDepthBias; },
		"offset", [](auto&& self) -> auto& { return self.data.offset; },
		"rotation", [](auto&& self) -> auto& { return self.data.rotation; },
		"externalEmittance", &T::emittanceFormEDID,
		"flags", &T::flags,
		"conditions", &T::conditions,
		"conditionalNodes", [](auto&& self) -> auto& { return self.data.conditionalNodes; },
		"colorController", &T::colorController,
		"radiusController", &T::radiusController,
		"fadeController", &T::fadeController,
		"positionController", &T::positionController,
		"rotationController", &T::rotationController,
		"lightController", &T::lightController);
};

struct REFR_LIGH
{
	struct Condition
	{
		enum UpdateFlags : std::uint8_t
		{
			Skip = 0,
			Normal = (1 << 0),
			Forced = (1 << 1),
			CellTransition = (1 << 2),
			Waiting = (1 << 3),

			UpdateRequired = CellTransition | Waiting
		};
	};
	using ConditionUpdateFlags = Condition::UpdateFlags;

	// cull nodes based on condition state
	struct NodeVisHelper
	{
		void InsertConditionalNodes(const StringSet& a_nodes, bool a_isVisble);
		void UpdateNodeVisibility(const RE::TESObjectREFR* a_ref, std::string_view a_nodeName);
		void Reset();

		// members
		bool            isVisible{ false };
		bool            canCullAddonNodes{ false };
		bool            canCullNodes{ false };
		StringMap<bool> conditionalNodes;
	};

	REFR_LIGH() = default;
	REFR_LIGH(const LightSourceData& a_lightSource, RE::BSLight* a_bsLight, RE::NiPointLight* a_niLight, RE::NiAVObject* a_debugMarker, RE::TESObjectREFR* a_ref, float a_scale);

	bool operator==(const REFR_LIGH& rhs) const
	{
		return niLight->name == rhs.niLight->name;
	}

	bool operator==(const RE::NiPointLight* rhs) const
	{
		return niLight->name == rhs->name;
	}

	const RE::NiPointer<RE::NiPointLight>& GetLight() const;

	void HideLight(bool a_hide, LightData::CullFlags a_flags) const;
	bool IsOutsideFrustum(bool a_freeCameraMode);
	bool DimLight(float a_dimmer) const;
	void ReattachLight(RE::TESObjectREFR* a_ref);
	void ReattachLight() const;
	void RemoveLight(bool a_clearData) const;
	void ShowDebugMarker() const;
	void HideDebugMarker() const;
	bool SetLightCullState(bool a_cull);
	bool ShouldUpdateConditions(ConditionUpdateFlags a_flags) const;
	void UpdateAnimation(float a_scalingFactor);
	void UpdateDebugMarkerState(bool a_culled) const;
	void UpdateConditions(RE::TESObjectREFR* a_ref, NodeVisHelper& a_nodeVisHelper, ConditionUpdateFlags a_flags);
	void UpdateEmittance() const;
	void UpdateVanillaFlickering() const;

	LightData                         data;
	RE::NiPointer<RE::BSLight>        bsLight;
	RE::NiPointer<RE::NiPointLight>   niLight;
	RE::NiPointer<RE::NiAVObject>     debugMarker;
	std::optional<ColorController>    colorController;
	std::optional<FloatController>    radiusController;
	std::optional<FloatController>    fadeController;
	std::optional<PositionController> positionController;
	std::optional<RotationController> rotationController;
	std::optional<AIOController>      lightController;
	float                             scale{ 1.0f };
	std::optional<bool>               lastVisibleState;
	bool                              culled{};

private:
	void CullDebugMarker(bool a_cull) const;
	void UpdateIndividualAnimations();
};

using ConditionUpdateFlags = REFR_LIGH::ConditionUpdateFlags;
