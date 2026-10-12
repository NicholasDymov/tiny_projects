#pragma once

#include <cstddef>
#include <memory>

template <typename T, std::size_t N, typename Allocator = std::allocator<T>>
class SmallVector {
	static_assert(N > 0, "N must be > 0");

  public:
	SmallVector() noexcept(noexcept(Allocator())) :
			SmallVector(Allocator()) {}

	explicit SmallVector(const Allocator &alloc) :
			alloc(alloc) {}

  private:
	using AllocTraits = std::allocator_traits<Allocator>;

	[[no_unique_address]] Allocator alloc;

	alignas(T) std::byte stack_storage[N * sizeof(T)];

	std::size_t size	 = 0;
	std::size_t capacity = N;
	T		   *data	 = reinterpret_cast<T *>(stack_storage);

	T *stack_ptr() noexcept {
		return reinterpret_cast<T *>(stack_storage);
	}

	const T *stack_ptr() const noexcept {
		return reinterpret_cast<const T *>(stack_storage);
	}

	bool is_small() const noexcept {
		return data == stack_ptr();
	}
};
