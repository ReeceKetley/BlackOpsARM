#pragma once
#include <cstdint>
#include <type_traits>
#include <atomic>
#include <platform/android/tl_thread_atomic.h>
#include <platform/win32_compat/win32_time.h>
namespace bo1::android::jq_atomic {
template<class T> T load(const volatile T *p) {return __atomic_load_n(p,__ATOMIC_SEQ_CST);}
template<class T,class D,class C> auto compareExchange(std::atomic<T> *p,D desired,C comparand) {
    T expected=static_cast<T>(comparand);p->compare_exchange_strong(expected,static_cast<T>(desired));
    return static_cast<std::conditional_t<sizeof(T)==4,std::int32_t,std::int64_t>>(expected);
}
template<class T,class D> std::int32_t add32(std::atomic<T> *p,D value) {
    static_assert(sizeof(T)==4);return static_cast<std::int32_t>(p->fetch_add(static_cast<T>(value)));
}
template<class T,class D,class C> auto compareExchange(volatile T *p,D desired,C comparand) {
    static_assert(std::is_integral_v<T> && (sizeof(T)==4 || sizeof(T)==8));
    T expected=static_cast<T>(comparand);
    __atomic_compare_exchange_n(p,&expected,static_cast<T>(desired),false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
    using Signed=std::conditional_t<sizeof(T)==4,std::int32_t,std::int64_t>;
    return static_cast<Signed>(expected);
}
template<class T,class D> auto exchange(volatile T *p,D desired) {
    static_assert(std::is_integral_v<T> && (sizeof(T)==4 || sizeof(T)==8));
    using Signed=std::conditional_t<sizeof(T)==4,std::int32_t,std::int64_t>;
    return static_cast<Signed>(__atomic_exchange_n(p,static_cast<T>(desired),__ATOMIC_SEQ_CST));
}
template<class T,class D> std::int32_t add32(volatile T *p,D value) {
    static_assert(std::is_integral_v<T> && sizeof(T)==4);
    return static_cast<std::int32_t>(__atomic_fetch_add(p,static_cast<T>(value),__ATOMIC_SEQ_CST));
}
template<class T> std::int32_t decrement32(volatile T *p) {
    static_assert(std::is_integral_v<T> && sizeof(T)==4);
    return static_cast<std::int32_t>(__atomic_sub_fetch(p,static_cast<T>(1),__ATOMIC_SEQ_CST));
}
inline void *comparePointer(void *volatile *p,void *desired,void *comparand) {
    auto expected=comparand;
    __atomic_compare_exchange_n(p,&expected,desired,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
    return expected;
}
}
