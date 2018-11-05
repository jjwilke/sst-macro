#ifndef sprockit_allocator_h
#define sprockit_allocator_h

#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <sprockit/thread_safe_new.h>

namespace sprockit {

template <class T>
class allocator
{
 public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;

  template <class U>
  struct rebind { typedef allocator<U> other; };


  size_t  unit_size;
  std::vector<T*> storage;
  std::vector<char*> allocations;

  allocator() {
    init();
  }

  allocator(const allocator&) {
    init();
  }
  
  ~allocator(){
    for (char* ptr : allocations){
      delete[] ptr;
    }
  }

  void init(){
    size_t rem = sizeof(T) % 32;
    if (rem){
      unit_size = sizeof(T) + 32 - rem;
    } else {
      unit_size = sizeof(T);
    }
  }

  void destroy(pointer p) { p->~T(); }

  pointer allocate(size_type n, const void * = 0) {
    //ignore hints
    static int constexpr increment = 1024;
    if (n > 1){
      ::abort();
      return (T*) new char[unit_size*n];
    } else {
      if (storage.empty()){
        char* ptr = new char[unit_size*increment];
        allocations.push_back(ptr);
        storage.resize(increment);
        for (int i=0; i < increment; ++i, ptr += unit_size)
          storage[i] = (T*) ptr;
      }
      T* ret = storage.back();
      storage.pop_back();
      return ret;
    }
  }

  void deallocate(void* p, size_type n) {
    if (n > 1){
      char* arr = (char*) p;
      delete[] arr;
    } else {
      storage.push_back((T*)p);
    }
  }

};

#if 0
template <class T> using thread_safe_allocator = std::allocator<T>;
#else
template <class T>
class thread_safe_allocator
{
 public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;

  template <class U>
  struct rebind { typedef thread_safe_allocator<U> other; };

  void destroy(pointer p) { p->~T(); }
  void construct(pointer p, const T& val){ new ((T*) p) T(val); }

  pointer allocate(size_type n, const void * = 0) {
    //ignore hints
    if (n > 1){
      std::cerr << "thread safe allocator cannot allocate more than 1 item at a time" << std::endl;
      ::abort();
    } else {
      return (pointer) thread_safe_new<T>::operator new(sizeof(T));
    }
  }

  void deallocate(void* p, size_type n) {
    if (n > 1){
      std::cerr << "thread safe allocator cannot allocate more than 1 item at a time" << std::endl;
      ::abort();
    } 
    thread_safe_new<T>::operator delete(p);
  }

};
#endif

}

#endif
