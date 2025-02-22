/*
 * Copyright 2015-2021 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

/*!
 * \file
 *
 * Utilities for aligning heap-allocated objects.
 */
#ifndef ARX_PLATFORM_ALIGNMENT_H
#define ARX_PLATFORM_ALIGNMENT_H

#include <cstddef>
#include <cstdint>
#include <new>
#include <memory>
#include <limits>
#include <type_traits>
#include <utility>

#include <boost/preprocessor/punctuation/comma.hpp>

#include "platform/PlatformConfig.h"

#include "platform/Platform.h"


/*!
 * \def arx_is_aligned(Pointer, aligned)
 * \brief Check if a pointer is aligned.
 */
#define arx_is_aligned(Pointer, Alignment) \
	(reinterpret_cast<std::uintptr_t>(Pointer) % (Alignment) == 0)

/*!
 * \def arx_return_aligned(Alignment)
 * \brief Declare that the pointer returned by a function has a specific alignment
 */
#if ARX_HAVE_ATTRIBUTE_ASSUME_ALIGNED
#define arx_return_aligned_impl(Alignment) __attribute__((assume_aligned(Alignment)))
#endif
#ifdef arx_return_aligned_impl
#define arx_return_aligned(Alignment) arx_return_aligned_impl(Alignment)
#else
#define arx_return_aligned(Alignment)
#endif

/*!
 * \def arx_assume_aligned(Pointer, Alignment)
 * \brief Assume that a pointer is aligned.
 *
 * In debug builds, alignment is checked using \ref arx_assert().
 *
 * Unlike arx_assert(Expression) this macro also tells the compiler to assume the pointer is aligned
 * in release builds.
 *
 * Depending on the compilter the alignment of the pointer is only assumed for the return value of the macro:
 * \code
 * const char * ptr = …;
 * const char * aligned = arx_assume_aligned(ptr, 16);
 * // Use aligned instead of ptr
 * \endcode
 */
#ifdef ARX_DEBUG
	template <typename T>
	T * checkAlignment(T * pointer, size_t alignment, const char * file, unsigned line) {
		if(!arx_is_aligned(pointer, alignment)) {
			assertionFailed("unaligned pointer", file, line, nullptr);
			arx_trap();
		}
		return pointer;
	}
	#define arx_assume_aligned(Pointer, Alignment) \
		checkAlignment((Pointer), (Alignment), ARX_FILE, __LINE__)
#elif ARX_HAVE_BUILTIN_ASSUME_ALIGNED
	#define arx_assume_aligned(Pointer, Alignment) \
		static_cast<decltype(Pointer)>(__builtin_assume_aligned((Pointer), (Alignment)))
#else
	// TODO Use lambda
	template <size_t Alignment, typename T>
	arx_return_aligned(Alignment)
	arx_force_inline
	T * assumeAlignment(T * pointer) {
		arx_assume(arx_is_aligned(pointer, Alignment));
		return pointer;
	}
	#define arx_assume_aligned(Pointer, Alignment) \
		assumeAlignment<(Alignment)>((Pointer))
#endif

/*!
 * \def arx_alloc_align(SizeArg, AlignArg)
 * \brief Annotate a function that returns a pointer whose alignment is given by parameter with index
 *        AlignArg and that doesn't alias with anything and points to uninitialized or zeroed memory of
 *        size given by the function parameter with index SizeArg
 */
#if ARX_HAVE_ATTRIBUTE_ALLOC_ALIGN
#define arx_alloc_align(SizeArg, AlignArg) arx_alloc(SizeArg) __attribute__((alloc_align(AlignArg)))
#else
#define arx_alloc_align(SizeArg, AlignArg) arx_alloc(SizeArg)
#endif

#define arx_alloc_align_static(SizeArg, Alignment) arx_alloc(SizeArg) arx_return_aligned(Alignment)

namespace platform {

/*!
 * Allocate a buffer with a specific alignment.
 *
 * This is only needed if the required alignment is greater than \c alignof(std::max_align_t).
 *
 * \param alignment The required alignment. This must be a power of two and
 *                  a multiple of \code sizeof( void *) \endcode.
 * \param size      The required buffer size.
 */
arx_alloc_align(2, 1)
void * alloc_aligned(std::size_t alignment, std::size_t size);

/*!
 * Free a pointer that was allocated with \ref alloc_aligned.
 *
 * \param ptr The pointer to free. This must either be \c nullptr or a pointer
 *            that was previously returned from \ref alloc_aligned and not yet freed.
 */
void free_aligned(void * ptr);

/*!
 * Allocation helper that uses aligned allocation if required but otherwise uses \c new.
 *
 * \tparam Alignment The required alignment. This must be a power of two and
 *                   a multiple of \code sizeof( void *) \endcode.
 */
template <size_t Alignment, bool NeedsManualAlignment = (Alignment > alignof(std::max_align_t))>
struct AlignedAllocator {
	static const void * void_type;
	static_assert(Alignment % sizeof(void_type) == 0,
	                  "Alignment must be a multiple of sizeof(void *)");
	static_assert((Alignment & (Alignment - 1)) == 0,
	                  "Alignment must be a power of two");
	arx_alloc_align_static(1, Alignment)
	static void * alloc_object(std::size_t size) {
		void * ptr = arx_assume_aligned(alloc_aligned(Alignment, size), Alignment);
		if(!ptr) {
			throw std::bad_alloc();
		}
		return ptr;
	}
	static void free_object(void * ptr) {
		free_aligned(ptr);
	}
	arx_alloc_align_static(1, Alignment)
	static void * alloc_array(std::size_t size) {
		return alloc_object(size);
	}
	static void free_array(void * ptr) {
		free_object(ptr);
	}
};

template <size_t Alignment>
struct AlignedAllocator<Alignment, false> {
	arx_alloc_align_static(1, alignof(std::max_align_t))
	static void * alloc_object(std::size_t size) {
		return ::operator new(size);
	}
	static void free_object(void * ptr) {
		::operator delete(ptr);
	}
	arx_alloc_align_static(1, alignof(std::max_align_t))
	static void * alloc_array(std::size_t size) {
		return ::operator new[](size);
	}
	static void free_array(void * ptr) {
		::operator delete[](ptr);
	}
};

/*!
 * C++ allocator with alignment support.
 *
 * \tparam T         The type to allocate.
 * \tparam Alignment The required alignment. Will use the alignment of T if not specified.
 */
template <typename T, size_t Alignment = alignof(T)>
struct aligned_allocator {
	
	typedef T value_type;
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T & reference;
	typedef const T & const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	template <class U> struct rebind { typedef std::allocator<U> other; };
	
	typedef std::true_type propagate_on_container_move_assignment;
	typedef std::true_type is_always_equal;
	
	aligned_allocator() noexcept = default;
	aligned_allocator(const aligned_allocator & o) noexcept { ARX_UNUSED(o); }
	template <typename U>
	aligned_allocator(const aligned_allocator<U> & o) noexcept { ARX_UNUSED(o); }
	
	pointer address(reference x) const noexcept { return &x; }
	const_pointer address(const_reference x) const noexcept { return &x; }
	
	size_type max_size() const {
		return std::numeric_limits<size_type>::max() / sizeof(value_type);
	}
	
	pointer allocate(size_type n, const void * hint = 0) {
		ARX_UNUSED(hint);
		if(n > max_size()) {
			throw std::bad_alloc();
		}
		return static_cast<pointer>(AlignedAllocator<Alignment>::alloc_array(n * sizeof(value_type)));
	}
	
	void deallocate(pointer p, size_type n) {
		ARX_UNUSED(n);
		AlignedAllocator<Alignment>::free_array(p);
	}
	
	template <class U, class... Args>
	void construct(U * p, Args &&... args) {
		::new(static_cast<void *>(p)) U(std::forward<Args>(args)...);
	}
	
	template <class U>
	void destroy(U * p) {
		p->~U();
	}
	
	friend bool operator!=(const aligned_allocator & a, const aligned_allocator & b) {
		ARX_UNUSED(a), ARX_UNUSED(b);
		return false;
	}
	
	friend bool operator==(const aligned_allocator & a, const aligned_allocator & b) {
		ARX_UNUSED(a), ARX_UNUSED(b);
		return true;
	}
	
};

//! Check if a pointer has aparticular alignment.
inline bool is_aligned_on(const void * p, size_t alignment) {
	return alignment == 1 || (size_t(p) % alignment == 0);
}

//! Check if a pointer is aligned for a specific type.
template <class T>
bool is_aligned(const void * p) {
	return is_aligned_on(p, alignof(T));
}

} // namespace platform


/*!
 * \def ARX_USE_ALIGNED_NEW_N(Alignment)
 * Override the new, new[], delete and delete[] operators to use a specific alignment.
 * 
 * This should go in the body of a class to only override the operators for that class.
 * 
 * \param Alignment The alignment to use. This must be a power of two and a multiple of
 *                  sizeof(void *).
 */
#define ARX_USE_ALIGNED_NEW_N(Alignment) \
	void * operator new(std::size_t size) { \
		return ::platform::AlignedAllocator<Alignment>::alloc_object(size); \
	} \
	void * operator new[](std::size_t size) { \
		return ::platform::AlignedAllocator<Alignment>::alloc_array(size); \
	} \
	void operator delete(void * ptr) { \
		::platform::AlignedAllocator<Alignment>::free_object(ptr); \
	} \
	void operator delete[](void * ptr) { \
		::platform::AlignedAllocator<Alignment>::free_array(ptr); \
	}

/*!
 * \def ARX_USE_ALIGNED_NEW(Class)
 * Override the new, new[], delete and delete[] operators to use the alignment required
 * by a class.
 * 
 * This should only be used in the global namespace.
 * 
 * \param Class The class whose alignment requirements should be used.
 */
#define ARX_USE_ALIGNED_NEW(Class) ARX_USE_ALIGNED_NEW_N(alignof(Class))

#define ARX_USE_ALIGNED_ALLOCATOR_T(Template, Class, Alignment) \
	namespace std { \
	template <Template> \
	struct allocator<Class> : public ::platform::aligned_allocator<Class, Alignment> { \
		allocator() noexcept { } \
		allocator(const allocator & o) noexcept \
			: ::platform::aligned_allocator<Class, Alignment>(o) { } \
		template <typename U> \
		allocator(const allocator<U> & o) noexcept \
			: ::platform::aligned_allocator<Class, Alignment>(o) { } \
		~allocator() noexcept { } \
	}; \
	}

/*!
 * \def ARX_USE_ALIGNED_ALLOCATOR_N(Class, Alignment)
 * Override the default std::allocator for a class to use a specific alignment.
 * 
 * This should only be used in the global namespace.
 * 
 * \param Class     The user-defined class to override std::allocator for.
 * \param Alignment The alignment to use. This must be a power of two and a multiple of
 *                  sizeof(void *).
 */
#define ARX_USE_ALIGNED_ALLOCATOR_N(Class, Alignment) \
	ARX_USE_ALIGNED_ALLOCATOR_T(typename Key, std::pair<Key BOOST_PP_COMMA() Class>, Alignment) \
	ARX_USE_ALIGNED_ALLOCATOR_T(typename Value, std::pair<Class BOOST_PP_COMMA() Value>, Alignment) \
	ARX_USE_ALIGNED_ALLOCATOR_T(, Class, Alignment)

/*!
 * \def ARX_USE_ALIGNED_ALLOCATOR_N(Class, Alignment)
 * Override the default std::allocator for a class to use the required alignment.
 * 
 * This should only be used in the global namespace.
 * 
 * \param Class     The user-defined class to override std::allocator for.
 */
#define ARX_USE_ALIGNED_ALLOCATOR(Class) \
	ARX_USE_ALIGNED_ALLOCATOR_N(Class, alignof(Class))


#endif // ARX_PLATFORM_ALIGNMENT_H
