#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mkmi.h>
#include "heap.hpp"

template<class T>
class safe_ptr {
	private:
		T *ptr = NULL;
		usize *refCount = NULL;
	public:
		safe_ptr() : ptr(NULL), refCount((usize*)malloc(sizeof(usize))) { *refCount = 1; }

		safe_ptr(T *ptr) : ptr(ptr), refCount((usize*)malloc(sizeof(usize))) { *refCount = 1; }

		// copy constructor
		safe_ptr(const safe_ptr &obj)
		{
			if (this->ptr == obj.ptr) {
				return;
			}

			this->ptr = obj.ptr; // share the underlying pointer
			this->refCount = obj.refCount; // share refCount
			if (obj.ptr != NULL)
			{
				// if the pointer is not null, increment the refCount
				(*this->refCount)++; 
			}
		}
		// copy assignment
		safe_ptr& operator=(const safe_ptr &obj)
		{
			if (this->ptr == obj.ptr) {
				return *this;
			}

			// cleanup any existing data
			// Assign incoming object's data to this object
			this->ptr = obj.ptr; // share the underlying pointer
			this->refCount = obj.refCount; // share refCount
			if (NULL != obj.ptr)
			{
				// if the pointer is not null, increment the refCount
				(*this->refCount)++; 
			}
			
			return *this;
		}
		T* operator->() const
		{
			return this->ptr;
		}
		T& operator*() const
		{
			return this->ptr;
		}

		usize get_count() const
		{
			return *refCount; // *this->refCount
		}
		T* get() const
		{
			return this->ptr;
		}

		~safe_ptr() {
			__cleanup__();
		}

	private:
		void __cleanup__() {
			if(*refCount == 0) {
				mkmi_log("Cleaning up non existent ref in safe_ptr!\r\n");
			} else {
				(*refCount)--;
			}

			if (*refCount == 0) {
				if (ptr != NULL) {
					free(ptr);
				}

				free(refCount);
			}
		}
};

template<class T>
safe_ptr<T> secure_malloc() {
	T *ptr = (T*)malloc(sizeof(T));
	return safe_ptr<T>(ptr);
}

template<class T>
safe_ptr<T> secure_malloc(usize size) {
	T *ptr = (T*)malloc(size);
	return safe_ptr<T>(ptr);
}
