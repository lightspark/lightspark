/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef UTILS_ANY_H
#define UTILS_ANY_H 1

#include <exception>

#include "utils/type_traits.h"
#include "utils/utility.h"

namespace lightspark
{

struct BadCast : std::exception
{
	const char* what() const noexcept override { return "bad cast"; }
};

struct BadAnyCast : std::exception
{
	const char* what() const noexcept override { return "bad any cast"; }
};

// Based on `any` from EASTL.
class Any
{
	template<typename T, typename U, typename... Args>
	using ConstructInitListType = EnableIf
	<
		std::is_constructible
		<
			T,
			std::initializer_list<U>&,
			Args...
		>::value
	>;
private:
	enum StorageOp
	{
		Get,
		Destroy,
		Copy,
		Move,
		TypeInfo,
	};

	union Storage
	{
		void* external { nullptr };
		alignas(void*) uint8_t internal[4 * sizeof(void*)];
	};

	// Determines when the "local buffer optimization" can be used.
	template<typename T>
	using UseInternalStorage = BoolConstant
	<
		std::is_nothrow_move_constructible<T>::value &&
		(sizeof(T) <= sizeof(Storage)) &&
		alignof(Storage) % alignof(T) == 0
	>;


	template<typename T>
	friend constexpr T anyCast(const Any& value);
	template<typename T>
	friend constexpr T anyCast(Any& value);
	template<typename T>
	friend constexpr T anyCast(Any&& value);
	template<typename T>
	friend constexpr const T* anyCast(const Any* value) noexcept;
	template<typename T>
	friend constexpr T* anyCast(Any* value) noexcept;

	template<typename T>
	friend constexpr T unsafeAnyCast(const Any& value) noexcept;
	template<typename T>
	friend constexpr T unsafeAnyCast(Any& value) noexcept;
	template<typename T>
	friend constexpr T unsafeAnyCast(Any&& value) noexcept;
	template<typename T>
	friend constexpr const T* unsafeAnyCast(const Any* value) noexcept;
	template<typename T>
	friend constexpr T* unsafeAnyCast(Any* value) noexcept;


	template<typename T>
	struct InternalStorageHandler
	{
		template<typename U>
		static constexpr void construct(Storage& storage, U&& value)
		{
			new(&storage.internal) T(std::forward<U>(value));
		}

		template<typename... Args>
		static constexpr void constructInplace(Storage& storage, Args&&... args)
		{
			new(&storage.internal) T(std::forward<Args>(args)...);
		}

		template<typename U, typename V, typename... Args>
		static constexpr void constructInplace
		(
			Storage& storage,
			const std::initializer_list<V>& list,
			Args&&... args
		)
		{
			new(&storage.internal) U(list, std::forward<Args>(args)...);
		}

		static constexpr void destroy(Any& any)
		{
			T& value = *static_cast<T*>(static_cast<void*>(&any.storage.internal));
			value.~T();
			any.handler = nullptr;
		}

		static constexpr void* handler(const StorageOp& op, const Any* self, Any* other)
		{
			if (op != StorageOp::TypeInfo)
				assert(self != nullptr);

			switch (op)
			{
				case StorageOp::Get:
					return (void*)&self->storage.internal;
					break;
				case StorageOp::Copy:
					assert(other != nullptr);
					construct(other->storage, *(T*)&self->storage.internal);
					break;
				case StorageOp::Move:
					assert(other != nullptr);
					construct(other->storage, std::move(*(T*)&self->storage.internal));
				// Falls through
				case StorageOp::Destroy:
					destroy(const_cast<Any&>(*self));
					break;
				case StorageOp::TypeInfo:
					return (void*)&typeid(T);
					break;
			}
			return nullptr;
		}
	};

	template<typename T>
	struct ExternalStorageHandler
	{
		template<typename U>
		static constexpr void construct(Storage& storage, U&& value)
		{
			storage.external = new T(std::forward<U>(value));
		}

		template<typename... Args>
		static constexpr void constructInplace(Storage& storage, Args&&... args)
		{
			storage.external = new T(std::forward<Args>(args)...);
		}

		template<typename U, typename V, typename... Args>
		static constexpr void constructInplace
		(
			Storage& storage,
			const std::initializer_list<V>& list,
			Args&&... args
		)
		{
			storage.external = new U(list, std::forward<Args>(args)...);
		}

		static constexpr void destroy(Any& any)
		{
			delete static_cast<T*>(any.storage.external);
			any.handler = nullptr;
		}

		static constexpr void* handler(const StorageOp& op, const Any* self, Any* other)
		{
			if (op != StorageOp::TypeInfo)
				assert(self != nullptr);

			switch (op)
			{
				case StorageOp::Get:
					assert(self->storage.external != nullptr);
					return self->storage.external;
					break;
				case StorageOp::Copy:
					assert(other != nullptr);
					construct(other->storage, *(T*)self->storage.external);
					break;
				case StorageOp::Move:
					assert(other != nullptr);
					construct(other->storage, std::move(*(T*)self->storage.external));
				// Falls through
				case StorageOp::Destroy:
					destroy(const_cast<Any&>(*self));
					break;
				case StorageOp::TypeInfo:
					return (void*)&typeid(T);
					break;
			}
			return nullptr;
		}
	};

	using StorageHandlerFunc = void* (*)(const StorageOp& op, const Any* self, Any* other);

	template<typename T>
	using StorageHandler = typename std::conditional
	<
		UseInternalStorage<T>::value,
		InternalStorageHandler<T>,
		ExternalStorageHandler<T>
	>::type;


	Storage storage;
	StorageHandlerFunc handler;
public:
	constexpr Any() noexcept : storage(), handler(nullptr) {}

	constexpr Any(const Any& other) : handler(nullptr)
	{
		if (other.handler != nullptr)
		{
			other.handler(StorageOp::Copy, &other, this);
			handler = other.handler;
		}
	}

	constexpr Any(Any&& other) noexcept : handler(nullptr)
	{
		if (other.handler != nullptr)
		{
			handler = std::move(other.handler);
			other.handler(StorageOp::Move, &other, this);
		}
	}

	template<typename T>
	constexpr Any(T&& value, EnableIf<!std::is_same<Decay<T>, Any>::value>* = nullptr)
	{
		static_assert(std::is_copy_constructible<Decay<T>>::value, "T must be copy constructible");
		StorageHandler<Decay<T>>::construct(storage, std::forward<T>(value));
		handler = &StorageHandler<Decay<T>>::handler;
	}

	template<typename T, typename... Args>
	constexpr explicit Any(InPlaceTypeTag<T>, Args&&... args)
	{
		static_assert(std::is_constructible<T, Args...>::value, "T must be constructible with Args...");
		StorageHandler<Decay<T>>::constructInplace(storage, std::forward<Args>(args)...);
		handler = &StorageHandler<Decay<T>>::handler;
	}

	template<typename T, typename U, typename... Args>
	constexpr explicit Any
	(
		InPlaceTypeTag<T>,
		const std::initializer_list<U>& list,
		Args&&... args,
		ConstructInitListType<T, U, Args...>* = nullptr
	)
	{
		StorageHandler<Decay<T>>::constructInplace(storage, list, std::forward<Args>(args)...);
		handler = &StorageHandler<Decay<T>>::handler;
	}

	template<typename T>
	constexpr Any& operator=(T&& value)
	{
		static_assert(std::is_copy_constructible<Decay<T>>::value, "T must be copy constructible");
		Any(std::forward<T>(value)).swap(*this);
		return *this;
	}

	constexpr Any& operator=(const Any& other)
	{
		Any(other).swap(*this);
		return *this;
	}

	constexpr Any& operator=(Any&& other) noexcept
	{
		Any(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T, typename... Args>
	constexpr void emplace(Args&&... args)
	{
		static_assert(std::is_constructible<T, Args...>::value, "T must be constructible with Args...");
		reset();
		StorageHandler<Decay<T>>::constructInplace(storage, std::forward<Args>(args)...);
		handler = &StorageHandler<Decay<T>>::handler;
	}

	template<typename T, typename U, typename... Args>
	constexpr ConstructInitListType<T, U, Args...>* emplace
	(
		const std::initializer_list<U>& list,
		Args&&... args
	)
	{
		reset();
		StorageHandler<Decay<T>>::constructInplace(storage, list, std::forward<Args>(args)...);
		handler = &StorageHandler<Decay<T>>::handler;
	}

	constexpr void reset() noexcept
	{
		if (handler != nullptr)
			handler(StorageOp::Destroy, this, nullptr);
	}

	constexpr void swap(Any& other) noexcept
	{
		if (this == &other)
			return;

		if (handler != nullptr && other.handler != nullptr)
		{
			Any tmp;
			tmp.handler = other.handler;
			other.handler(StorageOp::Move, &other, &tmp);

			other.handler = handler;
			handler(StorageOp::Move, this, &other);

			handler = tmp.handler;
			tmp.handler(StorageOp::Move, &tmp, this);

		}
		else if (handler != nullptr || other.handler != nullptr)
		{
			Any* to = handler != nullptr ? this : &other;
			Any* from = handler != nullptr ? &other : this;

			std::swap(to->handler, from->handler);
			to->handler(StorageOp::Move, to, from);
		}
	}

	constexpr bool hasValue() const noexcept {  return handler != nullptr; }

	constexpr const std::type_info& type() const noexcept
	{
		if (handler != nullptr)
			return *static_cast<const std::type_info*>(handler(StorageOp::TypeInfo, this, nullptr));
		return typeid(void);
	}

	template<typename T>
	constexpr T as()
	{
		static_assert
		(
			std::is_reference<T>::value ||
			std::is_copy_constructible<T>::value,
			"T must be a reference, or copy constructible"
		);
		auto* ptr = asPtr<RemoveRef<T>>();
		if (ptr == nullptr)
			throw BadAnyCast();
		return *ptr;
	}

	template<typename T>
	constexpr T as() const
	{
		static_assert
		(
			std::is_reference<T>::value ||
			std::is_copy_constructible<T>::value,
			"T must be a reference, or copy constructible"
		);
		auto* ptr = asPtr<RemoveRef<T>>();
		if (ptr == nullptr)
			throw BadAnyCast();
		return *ptr;
	}

	template<typename T>
	constexpr T* asPtr() noexcept
	{
		return
		(
			handler == &StorageHandler<Decay<T>>::handler &&
			type() == typeid(RemoveRef<T>)
		) ? unsafeAsPtr<T>() : nullptr;
	}

	template<typename T>
	constexpr T* asPtr() const noexcept
	{
		return
		(
			handler == &StorageHandler<Decay<T>>::handler &&
			type() == typeid(RemoveRef<T>)
		) ? unsafeAsPtr<T>() : nullptr;
	}

	template<typename T>
	constexpr T unsafeAs() noexcept
	{
		static_assert
		(
			std::is_reference<T>::value ||
			std::is_copy_constructible<T>::value,
			"T must be a reference, or copy constructible"
		);
		return *unsafeAsPtr<RemoveCVRef<T>>();
	}

	template<typename T>
	constexpr T unsafeAs() const noexcept
	{
		static_assert
		(
			std::is_reference<T>::value ||
			std::is_copy_constructible<T>::value,
			"T must be a reference, or copy constructible"
		);
		return *unsafeAsPtr<RemoveCVRef<T>>();
	}

	template<typename T>
	constexpr T* unsafeAsPtr() noexcept
	{
		return static_cast<T*>(handler(StorageOp::Get, this, nullptr));
	}

	template<typename T>
	constexpr T* unsafeAsPtr() const noexcept
	{
		return static_cast<T*>(handler(StorageOp::Get, this, nullptr));
	}
};

template<typename T>
constexpr T anyCast(const Any& value) { return value.as<T>(); }
template<typename T>
constexpr T anyCast(Any& value) { return value.as<T>(); }
template<typename T>
constexpr T anyCast(Any&& value) { return value.as<T>(); }
template<typename T>
constexpr const T* anyCast(const Any* value) noexcept { return value->asPtr<T>(); }
template<typename T>
constexpr T* anyCast(Any* value) noexcept { return value->asPtr<T>(); }

template<typename T>
constexpr T unsafeAnyCast(const Any& value) noexcept { return value.unsafeAs<T>(); }
template<typename T>
constexpr T unsafeAnyCast(Any& value) noexcept { return value.unsafeAs<T>(); }
template<typename T>
constexpr T unsafeAnyCast(Any&& value) noexcept { return value.unsafeAs<T>(); }
template<typename T>
constexpr const T* unsafeAnyCast(const Any* value) noexcept { return value->unsafeAsPtr<T>(); }
template<typename T>
constexpr T* unsafeAnyCast(Any* value) noexcept { return value->unsafeAsPtr<T>(); }


template<typename T, typename... Args>
constexpr Any makeAny(Args&&... args)
{
	return Any(InPlaceTypeTag<T> {}, std::forward<Args>(args)...);
}

template<typename T, typename U, typename... Args>
constexpr Any makeAny(const std::initializer_list<U>& list, Args&&... args)
{
	return Any(InPlaceTypeTag<T> {}, list, std::forward<Args>(args)...);
}

};
#endif /* UTILS_ANY_H */
