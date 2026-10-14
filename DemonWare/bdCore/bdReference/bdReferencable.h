#pragma once

#include <Windows.h>

template <typename T>
struct bdReference//<bdByteBuffer> // sizeof=0x4
{                                                                             // XREF: bdRemoteTask/r
    T *m_ptr;

    const bdReference<T> *operator=(const bdReference<T> &other)
    {
        if (&other != this)
        {
            if (m_ptr)
            {
                T *oldPtr = m_ptr;

#if defined(__ANDROID__)
                if (__atomic_sub_fetch(&m_ptr->m_refCount, 1u, __ATOMIC_ACQ_REL) == 0)
#else
                if (InterlockedDecrement(&m_ptr->m_refCount) == 0)
#endif
                {
                    // Demonware-style explicit destructor call
                    oldPtr->~T();
                }
            }

            m_ptr = other.m_ptr;

            if (m_ptr)
            {
#if defined(__ANDROID__)
                __atomic_add_fetch(&m_ptr->m_refCount, 1u, __ATOMIC_ACQ_REL);
#else
                InterlockedIncrement(&m_ptr->m_refCount);
#endif
            }
        }

        return this;
    }

    bdReference &operator=(T *ptr)
    {
        if (m_ptr != ptr)
        {
            if (m_ptr)
            {
                T *old = m_ptr;
#if defined(__ANDROID__)
                if (__atomic_sub_fetch(&old->m_refCount, 1u, __ATOMIC_ACQ_REL) == 0)
#else
                InterlockedDecrement(&old->m_refCount);
                if (!old->m_refCount)
#endif
                {
                    old->~T();
                }
            }

            m_ptr = ptr;

            if (m_ptr)
#if defined(__ANDROID__)
                __atomic_add_fetch(&m_ptr->m_refCount, 1u, __ATOMIC_ACQ_REL);
#else
                InterlockedIncrement(&m_ptr->m_refCount);
#endif
        }
        return *this;
    }
};

struct bdReferencable // sizeof=0x8
{                                                                             // XREF: bdByteBuffer/r
    volatile unsigned int m_refCount;

    virtual ~bdReferencable() = default;
};