#pragma once

#if defined(__ANDROID__)
#include <platform/android/tl_thread_atomic.h>
#define BO1_TL_THREAD_ID() bo1::android::tl_atomic::threadId()
#define BO1_TL_YIELD() bo1::android::tl_atomic::yield()
#define BO1_TL_CAS64(p,d,c) bo1::android::tl_atomic::compareExchange64(p,d,c)
#define BO1_TL_CAS32(p,d,c) bo1::atomic32::CmpExchange32(p,d,c)
#define BO1_TL_ADD32(p,n) bo1::atomic32::ExchangeAdd32(p,n)
#define BO1_TL_DEC32(p) bo1::atomic32::Decrement32(p)
#define BO1_TL_EXCHANGE(p,n) bo1::android::tl_atomic::exchangeBarrier(p,n)
#define BO1_TL_WORD std::int32_t
#else
#include <Windows.h>
#define BO1_TL_THREAD_ID() GetCurrentThreadId()
#define BO1_TL_YIELD() SwitchToThread()
#define BO1_TL_CAS64(p,d,c) _InterlockedCompareExchange64(p,d,c)
#define BO1_TL_CAS32(p,d,c) _InterlockedCompareExchange(p,d,c)
#define BO1_TL_ADD32(p,n) _InterlockedExchangeAdd(p,n)
#define BO1_TL_DEC32(p) _InterlockedDecrement(p)
#define BO1_TL_EXCHANGE(p,n) InterlockedExchange(p,n)
#define BO1_TL_WORD LONG
#endif

struct __declspec(align(8)) tlAtomicReadWriteMutex // sizeof=0x18
{                                       // XREF: .data:tlAtomicReadWriteMutex g_auto_rigid_body_map_mutex/r
                                        // broad_phase_memory/r
    volatile unsigned __int64 WriteThreadId;
                                        // XREF: _dynamic_initializer_for__g_auto_rigid_body_map_mutex__+3/w
                                        // _dynamic_initializer_for__g_auto_rigid_body_map_mutex__+D/w
    volatile unsigned int ReadLockCount;         // XREF: _dynamic_initializer_for__g_auto_rigid_body_map_mutex__+17/w
    volatile unsigned int WriteLockCount;        // XREF: _dynamic_initializer_for__g_auto_rigid_body_map_mutex__+21/w
    tlAtomicReadWriteMutex *ThisPtr;    // XREF: auto_rigid_body::add(centity_s const *,gjk_physics_collision_visitor *,int)+15B/r
                                        // _dynamic_initializer_for__g_auto_rigid_body_map_mutex__+2B/w ...
    // padding byte
    // padding byte
    // padding byte
    // padding byte

    tlAtomicReadWriteMutex()
    {
        WriteThreadId = 0;
        ReadLockCount = 0;
        WriteLockCount = 0;
        ThisPtr = this;
    }

    ~tlAtomicReadWriteMutex()
    {
        this->ThisPtr = 0;
    }

    inline void WriteLock() 
    {
        BO1_TL_WORD Target; // [esp+30h] [ebp-24h] BYREF
        volatile unsigned __int32 *p_WriteLockCount; // [esp+34h] [ebp-20h]
        tlAtomicReadWriteMutex *v4; // [esp+38h] [ebp-1Ch]
        volatile unsigned __int32 *p_ReadLockCount; // [esp+3Ch] [ebp-18h]
        tlAtomicReadWriteMutex *v6; // [esp+40h] [ebp-14h]
        tlAtomicReadWriteMutex *ThisPtr; // [esp+48h] [ebp-Ch]
        unsigned __int64 CurThread; // [esp+4Ch] [ebp-8h]

        CurThread = BO1_TL_THREAD_ID();
        ThisPtr = this->ThisPtr;
        if (BO1_TL_CAS64((volatile signed __int64 *)ThisPtr, CurThread, CurThread) == CurThread)
        {
            BO1_TL_ADD32(&this->ThisPtr->WriteLockCount, 1u);
            return;
        }
        while (1)
        {
            v6 = this->ThisPtr;
            if (BO1_TL_CAS64((volatile signed __int64 *)v6, CurThread, 0))
                goto LABEL_4;
            p_ReadLockCount = &this->ThisPtr->ReadLockCount;
            if (!BO1_TL_CAS32(p_ReadLockCount, 0, 0))
                break;
            do
                v4 = this->ThisPtr;
            while (BO1_TL_CAS64((volatile signed __int64 *)v4, 0, CurThread) != CurThread);
        LABEL_4:
            BO1_TL_YIELD();
        }
        p_WriteLockCount = &this->ThisPtr->WriteLockCount;
        BO1_TL_ADD32(p_WriteLockCount, 1u);
        Target = 0;
        BO1_TL_EXCHANGE(&Target, 0);
    }

    inline void ReadLock()
    {
        BO1_TL_WORD Target; // [esp+30h] [ebp-20h] BYREF
        tlAtomicReadWriteMutex *v3; // [esp+34h] [ebp-1Ch]
        volatile unsigned __int32 *v4; // [esp+38h] [ebp-18h]
        tlAtomicReadWriteMutex *v5; // [esp+3Ch] [ebp-14h]
        volatile unsigned __int32 *p_ReadLockCount; // [esp+40h] [ebp-10h]
        tlAtomicReadWriteMutex *ThisPtr; // [esp+44h] [ebp-Ch]
        unsigned __int64 CurThread; // [esp+48h] [ebp-8h]

        CurThread = BO1_TL_THREAD_ID();
        ThisPtr = this->ThisPtr;
        if (BO1_TL_CAS64((volatile signed __int64 *)ThisPtr, CurThread, CurThread) == CurThread)
        {
            p_ReadLockCount = &this->ThisPtr->ReadLockCount;
            BO1_TL_ADD32(p_ReadLockCount, 1u);
        }
        else
        {
            while (1)
            {
                v5 = this->ThisPtr;
                if (!BO1_TL_CAS64((volatile signed __int64 *)v5, CurThread, 0))
                    break;
                BO1_TL_YIELD();
            }
            v4 = &this->ThisPtr->ReadLockCount;
            BO1_TL_ADD32(v4, 1u);
            do
                v3 = this->ThisPtr;
            while (BO1_TL_CAS64((volatile signed __int64 *)v3, 0, CurThread) != CurThread);
        }
        Target = 0;
        BO1_TL_EXCHANGE(&Target, 0);
    }
    inline void ReadUnlock() {
#if defined(__ANDROID__)
        bo1::android::tl_atomic::readUnlock(&ThisPtr->ReadLockCount);
#else
        _InterlockedExchangeAdd(&ThisPtr->ReadLockCount, 0xFFFFFFFFu);
#endif
    }
    inline void WriteUnlock()
    {
        signed __int64 v1; // [esp+Ch] [ebp-20h]
        BO1_TL_WORD Target; // [esp+1Ch] [ebp-10h] BYREF
        volatile unsigned __int32 *p_WriteLockCount; // [esp+20h] [ebp-Ch]
        unsigned __int64 CurThread; // [esp+24h] [ebp-8h]

        CurThread = BO1_TL_THREAD_ID();
        p_WriteLockCount = &this->ThisPtr->WriteLockCount;
        if (!BO1_TL_DEC32(p_WriteLockCount))
        {
            Target = 0;
            BO1_TL_EXCHANGE(&Target, 0);
            do
                v1 = BO1_TL_CAS64((volatile signed __int64 *)this->ThisPtr, 0, CurThread);
            while (v1 != CurThread);
        }
    }
};
#undef BO1_TL_THREAD_ID
#undef BO1_TL_YIELD
#undef BO1_TL_CAS64
#undef BO1_TL_CAS32
#undef BO1_TL_ADD32
#undef BO1_TL_DEC32
#undef BO1_TL_EXCHANGE
#undef BO1_TL_WORD
