#include "phys_transient_allocator.h"
#include "phys_mem_new.h"
#include <cstdint>
#if !defined(__ANDROID__)
#include <intrin.h>
#endif

void phys_transient_allocator::reset()
{
    phys_transient_allocator::block_header *m_first_block; // eax
    phys_slot_pool *m_slot_pool; // ebx
    phys_transient_allocator::block_header *m_next_block; // esi

    m_first_block = this->m_first_block;
    m_slot_pool = (phys_slot_pool *)this->m_slot_pool;
    if ( this->m_first_block )
    {
        do
        {
            m_next_block = m_first_block->m_next_block;
            PSP_FREE(m_slot_pool, (unsigned __int8 *)m_first_block);
            m_first_block = m_next_block;
        }
        while ( m_next_block );
    }
    this->m_first_block = 0;
    this->m_cur = 0;
    this->m_end = 0;
    this->m_total_memory_allocated = 0;
}
void __thiscall phys_transient_allocator::reset_to_state(const phys_transient_allocator::allocator_state *as)
{
    phys_transient_allocator::block_header *m_first_block; // eax
    phys_transient_allocator::block_header *m_next_block; // esi
    phys_slot_pool *slot_pool; // [esp+8h] [ebp-4h]

    slot_pool = (phys_slot_pool *)this->m_slot_pool;
    m_first_block = this->m_first_block;
    if ( this->m_first_block != as->m_first_block )
    {
        do
        {
            m_next_block = m_first_block->m_next_block;
            PSP_FREE(slot_pool, (unsigned __int8 *)m_first_block);
            m_first_block = m_next_block;
        }
        while ( m_next_block != as->m_first_block );
    }
    *(phys_transient_allocator::allocator_state *)&this->m_first_block = *as;
}

void __thiscall phys_transient_allocator::resize()
{
    phys_slot_pool *m_slot_pool; // eax
    char *v3; // eax

    m_slot_pool = (phys_slot_pool *)this->m_slot_pool;
    if ( !m_slot_pool )
    {
        m_slot_pool = GET_PHYS_SLOT_POOL(0x4000u, 4u);
        this->m_slot_pool = m_slot_pool;
    }
    v3 = PSP_ALLOC(m_slot_pool);
    if ( v3 )
    {
        phys_transient_allocator::block_header *hdr = (phys_transient_allocator::block_header *)v3;
        hdr->m_block_size = 0x4000;
        hdr->m_block_alignment = 4;
        hdr->m_next_block = this->m_first_block;
        this->m_first_block = hdr;
        this->m_total_memory_allocated += 0x4000;
        this->m_cur = v3 + sizeof(phys_transient_allocator::block_header); // 12 on Win32, 16 on x64
        this->m_end = v3 + 0x4000;
    }
}

#if defined(__ANDROID__)
static inline bool TransientAllocationFits(char *cur, char *end, int size, int alignment, char *&aligned)
{
    // Empty/reset allocators must grow before any pointer arithmetic, including size zero.
    if (!cur || !end || size < 0 || alignment <= 0 || (alignment & (alignment - 1)))
        return false;

    const uintptr_t address = reinterpret_cast<uintptr_t>(cur);
    const uintptr_t endAddress = reinterpret_cast<uintptr_t>(end);
    const uintptr_t mask = static_cast<uintptr_t>(alignment - 1);
    if (address > endAddress || address > UINTPTR_MAX - mask)
        return false;

    const uintptr_t alignedAddress = (address + mask) & ~mask;
    if (alignedAddress > endAddress || static_cast<uintptr_t>(size) > endAddress - alignedAddress)
        return false;

    // The aligned cursor and requested extent are now within the current owned block.
    aligned = cur + (alignedAddress - address);
    return true;
}
#endif

static inline char *TransientAlignUp(char *p, int alignment)
{
    return (char *)(~(uintptr_t)(alignment - 1) & ((uintptr_t)p + (uintptr_t)(alignment - 1)));
}

char *phys_transient_allocator::mt_allocate_internal(int size, int alignment)
{
    char *cur;
    char *aligned;

    do
    {
#if defined(__ANDROID__)
        cur = __atomic_load_n(&this->m_cur, __ATOMIC_SEQ_CST);
        if (!TransientAllocationFits(cur, this->m_end, size, alignment, aligned))
            return nullptr;
#else
        cur = this->m_cur;
        aligned = TransientAlignUp(cur, alignment);
        if (aligned + size > this->m_end)
            return 0;
#endif
    } while (_InterlockedCompareExchangePointer((void *volatile *)&this->m_cur, aligned + size, cur) != cur);
    return aligned;
}

void *__thiscall phys_transient_allocator::allocate(
    int size,
    int alignment,
    int no_error,
    const char *error_msg)
{
    char *v7;
    char *v8;
    void *ptr;

    transient_allocator_update_largest_size();
#if defined(__ANDROID__)
    if (TransientAllocationFits(this->m_cur, this->m_end, size, alignment, v8))
#else
    v8 = TransientAlignUp(this->m_cur, alignment);
    if (v8 + size <= this->m_end)
#endif
    {
        this->m_cur = v8 + size;
        ptr = v8;
    }
    else
    {
        ptr = 0;
    }
    if (!ptr)
    {
        //phys_transient_allocator::resize();
        this->resize();
#if defined(__ANDROID__)
        if (TransientAllocationFits(this->m_cur, this->m_end, size, alignment, v7))
#else
        v7 = TransientAlignUp(this->m_cur, alignment);
        if (v7 + size <= this->m_end)
#endif
        {
            this->m_cur = v7 + size;
            ptr = v7;
        }
        else
        {
            ptr = 0;
        }
        if (!ptr
            && _tlAssert(
                "C:\\projects_pc\\cod\\codsrc\\tl\\physics\\include\\phys_transient_allocator.h",
                79,
                "ptr",
                "transient allocation too large, increase block_size."))
        {
            __debugbreak();
        }
    }
    if (!ptr
        && !no_error
        && _tlAssert(
            "C:\\projects_pc\\cod\\codsrc\\tl\\physics\\include\\phys_transient_allocator.h",
            81,
            "ptr || no_error",
            error_msg))
    {
        __debugbreak();
    }
    return ptr;
}

void *__thiscall phys_transient_allocator::mt_allocate(
    int size,
    int alignment,
    int no_error,
    const char *error_msg)
{
    char *v7;
    char *v8;
    void *ptr;

    transient_allocator_update_largest_size();
    //minspec_read_write_mutex::ReadLock(&this->m_mutex);
    this->m_mutex.ReadLock();
    ptr = phys_transient_allocator::mt_allocate_internal(size, alignment);
    //minspec_read_write_mutex::ReadUnlock(&this->m_mutex);
    this->m_mutex.ReadUnlock();
    if (!ptr)
    {
        //minspec_read_write_mutex::WriteLock(&this->m_mutex);
        this->m_mutex.WriteLock();
#if defined(__ANDROID__)
        if (TransientAllocationFits(this->m_cur, this->m_end, size, alignment, v8))
#else
        v8 = TransientAlignUp(this->m_cur, alignment);
        if (v8 + size <= this->m_end)
#endif
        {
            this->m_cur = v8 + size;
            ptr = v8;
        }
        else
        {
            ptr = 0;
        }
        if (!ptr)
        {
            phys_transient_allocator::resize();
#if defined(__ANDROID__)
            if (TransientAllocationFits(this->m_cur, this->m_end, size, alignment, v7))
#else
            v7 = TransientAlignUp(this->m_cur, alignment);
            if (v7 + size <= this->m_end)
#endif
            {
                this->m_cur = v7 + size;
                ptr = v7;
            }
            else
            {
                ptr = 0;
            }
            if (!ptr
                && _tlAssert(
                    "c:\\projects_pc\\cod\\codsrc\\tl\\physics\\include\\phys_transient_allocator.h",
                    99,
                    "ptr",
                    "transient allocation too large, increase block_size."))
            {
                __debugbreak();
            }
        }
        //minspec_read_write_mutex::WriteUnlock(&this->m_mutex);
        this->m_mutex.WriteUnlock();
    }
    if (!ptr
        && !no_error
        && _tlAssert(
            "c:\\projects_pc\\cod\\codsrc\\tl\\physics\\include\\phys_transient_allocator.h",
            103,
            "ptr || no_error",
            error_msg))
    {
        __debugbreak();
    }
    return ptr;
}
