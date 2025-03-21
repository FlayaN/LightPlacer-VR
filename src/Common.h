#pragma once

// for visting variants
template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};

// https://www.reddit.com/r/cpp/comments/p132c7/a_c_locking_wrapper/h8b8nml/
template <
	class T,
	class M = std::mutex,
	template <class...> class RL = std::unique_lock,
	template <class...> class WL = std::unique_lock>
struct MutexGuard
{
	MutexGuard() = default;
	explicit MutexGuard(T in) :
		data(std::move(in))
	{}
	template <class... Args>
	explicit MutexGuard(Args&&... args) :
		data(std::forward<Args>(args)...)
	{}
	~MutexGuard() = default;

	MutexGuard(const MutexGuard&) = delete;
	MutexGuard(MutexGuard&&) noexcept = default;
	MutexGuard& operator=(const MutexGuard&) = delete;
	MutexGuard& operator=(MutexGuard&&) noexcept = default;

	auto read(auto&& f) const
	{
		auto l = lock();
		return f(data);
	}
	auto read_unsafe(auto&& f)
	{
		auto l = lock_unsafe();
		return f(data);
	}
	auto write(auto&& f)
	{
		auto l = lock();
		return f(data);
	}

private:
	mutable std::unique_ptr<M> mutex{ std::make_unique<M>() };
	T                          data;

	auto lock() { return WL<M>(*mutex); }
	auto lock() const { return RL<M>(*mutex); }
	auto lock_unsafe() { return RL<M>(*mutex); }
};

template <class T>
using MutexGuardShared = MutexGuard<T, std::shared_mutex, std::shared_lock>;

template <class K, class D, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using FlatMap = boost::unordered_flat_map<K, D, H, KEqual>;

template <class K, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using FlatSet = boost::unordered_flat_set<K, H, KEqual>;

template <class K, class D, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using LockedMap = MutexGuardShared<FlatMap<K, D, H, KEqual>>;

struct string_hash
{
	using is_transparent = void;  // enable heterogeneous overloads

	std::size_t operator()(std::string_view str) const
	{
		std::size_t seed = 0;
		for (auto it = str.begin(); it != str.end(); ++it) {
			boost::hash_combine(seed, std::tolower(*it));
		}
		return seed;
	}
};

struct string_cmp
{
	using is_transparent = void;  // enable heterogeneous overloads

	bool operator()(const std::string& str1, const std::string& str2) const
	{
		return string::iequals(str1, str2);
	}
	bool operator()(std::string_view str1, std::string_view str2) const
	{
		return string::iequals(str1, str2);
	}
};

template <class D>
using StringMap = FlatMap<std::string, D, string_hash, string_cmp>;

using StringSet = FlatSet<std::string, string_hash, string_cmp>;

template <class T>
struct NiPointer_Hash
{
	using is_transparent = void;  // enable heterogeneous overloads

	std::size_t operator()(T* ptr) const
	{
		return boost::hash<T*>()(ptr);
	}
	std::size_t operator()(const RE::NiPointer<T>& ptr) const
	{
		return boost::hash<T*>()(ptr.get());
	}
};

template <class T>
struct NiPointer_Cmp
{
	using is_transparent = void;  // enable heterogeneous overloads

	bool operator()(T* lhs, const RE::NiPointer<T>& rhs) const
	{
		return lhs == rhs.get();
	}
	bool operator()(const RE::NiPointer<T>& lhs, const RE::NiPointer<T>& rhs) const
	{
		return lhs == rhs;
	}
};

template <class K, class D>
using LockedNiPtrMap = MutexGuardShared<FlatMap<RE::NiPointer<K>, D, NiPointer_Hash<K>, NiPointer_Cmp<K>>>;

namespace stl
{
	template <typename T, typename... Keys>
	bool contains(const FlatSet<T>& set, Keys... keys)
	{
		return (... || (set.contains(keys)));
	}
}
