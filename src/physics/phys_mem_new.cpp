#include <platform/bo1_native_width.h>
#include "phys_mem_new.h"

#include <Windows.h> // interlockedexchange
#include <new>
#include <universal/q_shared.h>
#if defined(__ANDROID__)
#include <platform/android/physics_atomic.h>
#include <cerrno>
#include <cstdlib>
#include <platform/android/oom_error.h>
#define InterlockedExchange _InterlockedExchange
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#endif

void *g_phys_memory_buffer;
int g_phys_memory_buffer_size;
phys_memory_manager *g_phys_memory_manager;

static constexpr uintptr_t kPmmAllocOwnerMagic = 0xFEDCBA98u;

static unsigned int pmm_ptr_to_bits(void *p)
{
    if (!p)
        return 0;
#if BO1_NATIVE_64BIT
    const uintptr_t base = reinterpret_cast<uintptr_t>(g_phys_memory_buffer);
    const uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    if ((addr < base || addr - base > static_cast<uintptr_t>(g_phys_memory_buffer_size))
        && _tlAssert("source/phys_mem_new.cpp", 1, "pmm slot in buffer", ""))
        __debugbreak();
    return static_cast<unsigned int>(addr - base);
#else
    return static_cast<unsigned int>(reinterpret_cast<uintptr_t>(p));
#endif
}

static void *pmm_bits_to_ptr(unsigned int bits)
{
    if (!bits)
        return nullptr;
#if BO1_NATIVE_64BIT
    return static_cast<char *>(g_phys_memory_buffer) + bits;
#else
    return reinterpret_cast<void *>(static_cast<uintptr_t>(bits));
#endif
}

void __cdecl phys_memory_manager_term()
{
    g_phys_memory_buffer = 0;
    g_phys_memory_buffer_size = 0;
    g_phys_memory_manager = 0;
}

void phys_slot_pool::init(unsigned int slot_size, unsigned int slot_alignment)
{
    this->m_first_free_slot.m_ptr = 0;
    this->m_first_free_slot.m_tag = 0;
    this->m_map_key = phys_slot_pool::encode_size_alignment(slot_size, slot_alignment);
    this->m_total_slot_count = 0;
    this->m_allocated_slot_count = 0;
}

void __cdecl ppu_pmm_get_linear_buffer(char ***linear_buffer_cur, char **linear_buffer_end)
{
    *linear_buffer_cur = &g_phys_memory_manager->m_buffer_cur;
    *linear_buffer_end = g_phys_memory_manager->m_buffer_end;
}

void __cdecl PSP_FREE(phys_slot_pool *slot_pool, unsigned __int8 *slot)
{
    slot_pool->free_slot(slot);
}

void __cdecl transient_allocator_update_largest_size()
{
    ;
}

void *phys_memory_manager::allocate(unsigned int size, unsigned int alignment)
{
#if defined(__ANDROID__)
    if (!alignment || (alignment&(alignment-1))) { errno=EINVAL; return nullptr; }
#endif
    // Pointer-width CAS (Win32 used 32-bit InterlockedCompareExchange on m_buffer_cur).
    const uintptr_t mask = ~(static_cast<uintptr_t>(alignment) - 1);
    const uintptr_t alignMinus1 = static_cast<uintptr_t>(alignment) - 1;

    for (;;)
    {
#if defined(__ANDROID__)
        char *cur = __atomic_load_n(&this->m_buffer_cur,__ATOMIC_SEQ_CST);
#else
        char *cur = this->m_buffer_cur;
#endif
#if defined(__ANDROID__)
        if (reinterpret_cast<uintptr_t>(cur)>UINTPTR_MAX-alignMinus1) return nullptr;
#endif
        const uintptr_t aligned = (reinterpret_cast<uintptr_t>(cur) + alignMinus1) & mask;
        char *next = reinterpret_cast<char *>(aligned + size);
#if defined(__ANDROID__)
        const auto end=reinterpret_cast<uintptr_t>(this->m_buffer_end);
        if (aligned>end || size>end-aligned)
#else
        if ( next > this->m_buffer_end )
#endif
            return nullptr;
#if defined(__ANDROID__)
        char *expected=cur;
        if (__atomic_compare_exchange_n(&this->m_buffer_cur,&expected,next,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST))
#else
        if ( InterlockedCompareExchangePointer(
                 reinterpret_cast<PVOID *>(&this->m_buffer_cur),
                 next,
                 cur) == cur )
#endif
            return reinterpret_cast<void *>(aligned);
    }
}

phys_slot_pool *phys_memory_manager::allocate_slot_pool()
{
    minspec_mutex *p_m_slot_pool_allocate_mutex; // ebx
    int m_list_preallocated_slot_pools_count; // eax
    phys_slot_pool *v4; // edi
    volatile unsigned int Target; // [esp+Ch] [ebp-4h] BYREF

    p_m_slot_pool_allocate_mutex = &this->m_slot_pool_allocate_mutex;
    while ( _InterlockedCompareExchange((volatile unsigned __int32 *)p_m_slot_pool_allocate_mutex, 1, 0) )
        ;
    Target = 0;
    InterlockedExchange(&Target, 0);
    m_list_preallocated_slot_pools_count = this->m_list_preallocated_slot_pools_count;
    if ( m_list_preallocated_slot_pools_count >= 28 )
    {
        v4 = static_cast<phys_slot_pool *>(this->allocate(sizeof(phys_slot_pool), 8));
        ++this->m_list_slot_pool_count;
    }
    else
    {
        ++this->m_list_slot_pool_count;
        v4 = &this->m_list_preallocated_slot_pools[m_list_preallocated_slot_pools_count];
        this->m_list_preallocated_slot_pools_count = m_list_preallocated_slot_pools_count + 1;
    }
    p_m_slot_pool_allocate_mutex->Unlock();
    //minspec_mutex::Unlock(p_m_slot_pool_allocate_mutex);
    return v4;
}

phys_slot_pool *__thiscall phys_memory_manager::get_slot_pool(
                unsigned int slot_size,
                unsigned int slot_alignment)
{
    unsigned int v5; // ebx
    minspec_hash_table<phys_slot_pool,64> *p_m_slot_pool_map; // edi
    phys_slot_pool *i; // esi
    phys_slot_pool *j; // edx
    minspec_read_write_mutex *p_m_slot_pool_map_mutex; // [esp+10h] [ebp-4h]
    unsigned int slot_sizea; // [esp+1Ch] [ebp+8h]

    if ( slot_alignment < 4 && _tlAssert("source/phys_mem_new.cpp", 204, "slot_alignment >= 4", "") )
        __debugbreak();
    if ( slot_size % slot_alignment
        && _tlAssert("source/phys_mem_new.cpp", 205, "slot_size % slot_alignment == 0", "") )
    {
        __debugbreak();
    }
    if ( (slot_alignment & 3) != 0
        && _tlAssert(
                 "source/phys_mem_new.cpp",
                 208,
                 "slot_alignment % PHYS_ALIGNOF(phys_slot_pool::extra_info) == 0",
                 "") )
    {
        __debugbreak();
    }
#if BO1_NATIVE_64BIT
    // Footer holds native pointers; keep slot base/size 8-aligned for Interlocked*Pointer.
    if ( slot_alignment < 8 )
        slot_alignment = 8;
    slot_size = (slot_size + 7u) & ~7u;
#endif
    // Win32 added 8 for a two-dword footer; use native sizeof(extra_info).
    slot_sizea = slot_size + static_cast<unsigned int>(sizeof(phys_slot_pool::extra_info));
    v5 = phys_slot_pool::encode_size_alignment(slot_sizea, slot_alignment);
    p_m_slot_pool_map_mutex = &this->m_slot_pool_map_mutex;
    //minspec_read_write_mutex::ReadLock(&this->m_slot_pool_map_mutex);
    this->m_slot_pool_map_mutex.ReadLock();
    p_m_slot_pool_map = &this->m_slot_pool_map;
    for ( i = p_m_slot_pool_map->m_hash_table[v5 % p_m_slot_pool_map->m_mod]; i; i = i->m_hash_next )
    {
        if ( i->m_map_key == v5 )
            break;
    }
    //minspec_read_write_mutex::ReadUnlock(p_m_slot_pool_map_mutex);
    p_m_slot_pool_map_mutex->ReadUnlock();
    if ( !i )
    {
        //minspec_read_write_mutex::WriteLock(p_m_slot_pool_map_mutex);
        p_m_slot_pool_map_mutex->WriteLock();
        for ( j = p_m_slot_pool_map->m_hash_table[v5 % p_m_slot_pool_map->m_mod]; j; j = j->m_hash_next )
        {
            if ( j->m_map_key == v5 )
                break;
        }
        i = j;
        if ( !j )
        {
            //i = phys_memory_manager::allocate_slot_pool(this);
            i = this->allocate_slot_pool();
            i->m_first_free_slot.m_ptr = 0;
            i->m_first_free_slot.m_tag = 0;
            i->m_map_key = phys_slot_pool::encode_size_alignment(slot_sizea, slot_alignment);
            i->m_total_slot_count = 0;
            i->m_allocated_slot_count = 0;
            //minspec_hash_table<phys_slot_pool,64>::add(p_m_slot_pool_map, v5, i);
            p_m_slot_pool_map->add(v5, i);
        }
        //minspec_read_write_mutex::WriteUnlock(p_m_slot_pool_map_mutex);
        p_m_slot_pool_map_mutex->WriteUnlock();
    }
    return i;
}

void *__cdecl PMM_PERM_ALLOCATE(unsigned int size, unsigned int alignment)
{
    void *result; // eax

#if defined(__ANDROID__)
    if (!g_phys_memory_manager || !alignment || (alignment & (alignment - 1)))
    {
        _tlAssert(__FILE__, __LINE__, "initialized physics arena and power-of-two alignment", "invalid permanent physics allocation");
        std::abort();
    }
#endif
    result = g_phys_memory_manager->allocate(size, alignment);
    if ( !result )
    {
#if defined(__ANDROID__)
        bo1::android::ReportAllocationFailure(__FILE__, __LINE__, "PMM_PERM_ALLOCATE", "physics arena", size, 0, false);
#endif
        if ( _tlAssert("source/phys_mem_new.cpp", 265, "ptr", "physics memory manager error: out of memory.") )
            __debugbreak();
        return 0;
    }
    return result;
}

char *__cdecl PMM_ALLOC(unsigned int size, unsigned int alignment)
{
    phys_slot_pool *slot_pool; // eax

    if ( !size && _tlAssert("source/phys_mem_new.cpp", 465, "size > 0", "") )
        __debugbreak();
    //slot_pool = phys_memory_manager::get_slot_pool(g_phys_memory_manager, size, alignment);
    slot_pool = g_phys_memory_manager->get_slot_pool(size, alignment);
    //return phys_slot_pool::allocate_slot(slot_pool, (unsigned int)&savedregs);
    return slot_pool->allocate_slot();
}

void __cdecl PMM_FREE(unsigned __int8 *ptr, unsigned int size, unsigned int alignment)
{
    phys_slot_pool *slot_pool; // eax

    //slot_pool = phys_memory_manager::get_slot_pool(g_phys_memory_manager, size, alignment);
    slot_pool = g_phys_memory_manager->get_slot_pool(size, alignment);
    //phys_slot_pool::free_slot(slot_pool, (unsigned int)&savedregs, ptr);
    slot_pool->free_slot(ptr);
}

char *__cdecl PSP_ALLOC(phys_slot_pool *slot_pool)
{
    //return phys_slot_pool::allocate_slot(slot_pool, (unsigned int)&savedregs);
    return slot_pool->allocate_slot();
}

phys_slot_pool *__cdecl GET_PHYS_SLOT_POOL(unsigned int size, unsigned int alignment)
{
    if ( !size && _tlAssert("source/phys_mem_new.cpp", 489, "size > 0", "") )
        __debugbreak();
    //return phys_memory_manager::get_slot_pool(g_phys_memory_manager, size, alignment);
    return g_phys_memory_manager->get_slot_pool(size, alignment);
}

void __cdecl PMM_VALIDATE(char *ptr, unsigned int size, unsigned int alignment)
{
    phys_slot_pool *slot_pool; // eax

    slot_pool = g_phys_memory_manager->get_slot_pool(size, alignment);
    slot_pool->validate_slot(ptr);
}

phys_memory_manager::phys_memory_manager(char *memory_buffer, int memory_buffer_size)
{
    this->m_slot_pool_map_mutex.m_count = 1;
#if defined(__ANDROID__)
    memset(this->m_slot_pool_map.m_hash_table, 0, sizeof(this->m_slot_pool_map.m_hash_table));
#else
    memset(&this->m_slot_pool_map, 0, 0x100u);
#endif
    this->m_slot_pool_map.m_mod = 1;
    this->m_slot_pool_map.m_highest_collision = 0;
    this->m_slot_pool_map.m_total_collisions = 0;
    this->m_slot_pool_allocate_mutex.m_token = 0;
    this->m_buffer_start = memory_buffer;
    this->m_buffer_cur = memory_buffer;
    this->m_buffer_end = &memory_buffer[memory_buffer_size];
    this->m_list_slot_pool_count = 0;
#if defined(__ANDROID__)
    memset(this->m_slot_pool_map.m_hash_table, 0, sizeof(this->m_slot_pool_map.m_hash_table));
#else
    memset(&this->m_slot_pool_map, 0, 0x100u);
#endif
    this->m_slot_pool_map.m_highest_collision = 0;
    this->m_slot_pool_map.m_total_collisions = 0;
    this->m_slot_pool_map.m_mod = 1;
    this->m_list_preallocated_slot_pools_count = 0;
}

void __cdecl phys_memory_manager_init(void *const memory_buffer, int memory_buffer_size)
{
    if ( g_phys_memory_buffer
        && _tlAssert("source/phys_mem_new.cpp", 237, "g_phys_memory_buffer == NULL", "") )
    {
        __debugbreak();
    }
    if ( g_phys_memory_buffer_size
        && _tlAssert("source/phys_mem_new.cpp", 238, "g_phys_memory_buffer_size == 0", "") )
    {
        __debugbreak();
    }
    if ( g_phys_memory_manager
        && _tlAssert("source/phys_mem_new.cpp", 239, "g_phys_memory_manager == NULL", "") )
    {
        __debugbreak();
    }
#if defined(__ANDROID__)
    const uintptr_t buf = reinterpret_cast<uintptr_t>(memory_buffer);
    constexpr uintptr_t mask = alignof(phys_memory_manager) - 1;
    if (g_phys_memory_buffer || g_phys_memory_buffer_size || g_phys_memory_manager || !buf ||
        memory_buffer_size <= 0 || buf > UINTPTR_MAX - static_cast<uintptr_t>(memory_buffer_size) || buf > UINTPTR_MAX - mask)
    {
        _tlAssert(__FILE__, __LINE__, "unowned, non-null, valid physics arena span", "invalid physics arena initialization");
        std::abort();
    }
    const uintptr_t aligned = (buf + mask) & ~mask;
    const uintptr_t end = buf + static_cast<uintptr_t>(memory_buffer_size);
    if (aligned > end || sizeof(phys_memory_manager) >= end - aligned)
    {
        _tlAssert(__FILE__, __LINE__, "physics arena has manager and allocation storage", "physics arena too small");
        std::abort();
    }
    auto *manager = new (reinterpret_cast<void *>(aligned)) phys_memory_manager(
        reinterpret_cast<char *>(aligned + sizeof(phys_memory_manager)),
        static_cast<int>(end - aligned - sizeof(phys_memory_manager)));
    // Constructor does not use the global arena. Publish only its complete state.
    g_phys_memory_buffer = memory_buffer;
    g_phys_memory_buffer_size = memory_buffer_size;
    g_phys_memory_manager = manager;
#else
    {
        const uintptr_t buf = reinterpret_cast<uintptr_t>(memory_buffer);
        const uintptr_t aligned = (buf + 7) & ~static_cast<uintptr_t>(7);
        g_phys_memory_buffer = memory_buffer;
        g_phys_memory_buffer_size = memory_buffer_size;
        g_phys_memory_manager = reinterpret_cast<phys_memory_manager *>(aligned);
        if (aligned)
        {
            const int poolBytes = static_cast<int>(
                (buf + static_cast<uintptr_t>(memory_buffer_size)) - aligned - sizeof(phys_memory_manager));
            new (reinterpret_cast<void *>(aligned)) phys_memory_manager(
                reinterpret_cast<char *>(aligned + sizeof(phys_memory_manager)), poolBytes);
        }
    }
#endif
}

void phys_slot_pool::extra_info_init(char *slot)
{
    phys_slot_pool::extra_info *ei; // eax

    if ( slot )
    {
        ei = (phys_slot_pool::extra_info *)&slot[(unsigned __int16)this->m_map_key - sizeof(phys_slot_pool::extra_info)];
        ei->m_slot_pool_owner = this;
        ei->m_allocation_owner = reinterpret_cast<void *>(kPmmAllocOwnerMagic);

        _InterlockedExchangeAdd(&this->m_total_slot_count, 1u);
        _InterlockedExchangeAdd(&this->m_allocated_slot_count, 1u);

        if ( this->m_allocated_slot_count > this->m_total_slot_count )
        {
            if ( _tlAssert(
                         "source/phys_mem_new.cpp",
                         296,
                         "GetStuff32(&m_allocated_slot_count) <= GetStuff32(&m_total_slot_count)",
                         "internal error.") )
            {
                __debugbreak();
            }
        }

        slot = 0;
#if defined(__ANDROID__)
        bo1::android::physics_atomic::memoryBarrier();
#else
        InterlockedExchange((volatile unsigned int *)&slot, 0);
#endif
    }
}

void __thiscall phys_slot_pool::extra_info_allocate(char *slot)
{
#if 0
    char *v3; // ebx
    unsigned int m_map_key; // edi
    bool v5; // zf
    int v6; // edi
    volatile unsigned int Target; // [esp+Ch] [ebp-4h] BYREF

    Target = 0;
    InterlockedExchange(&Target, 0);
    v3 = slot;
    m_map_key = this->m_map_key;
    if ( !slot && _tlAssert("source/phys_mem_new.cpp", 306, "slot", "") )
        __debugbreak();
    v5 = *(unsigned int *)&v3[(unsigned __int16)m_map_key - 8] == (unsigned int)this;
    v6 = (int)&v3[(unsigned __int16)m_map_key - 8];
    if ( !v5
        && _tlAssert("source/phys_mem_new.cpp", 309, "GetStuff32(&ei->m_slot_pool_owner) == this_", "internal error.") )
    {
        __debugbreak();
    }
    if ( (phys_slot_pool *)_InterlockedCompareExchange(
                                                     (volatile unsigned __int32 *)(v6 + 4),
                                                      0xFEDCBA98,
                                                     (signed __int32)this) != this
        && _tlAssert("source/phys_mem_new.cpp", 312, "retv", "internal error.") )
    {
        __debugbreak();
    }
    _InterlockedExchangeAdd(&this->m_allocated_slot_count, 1u);
    if ( this->m_allocated_slot_count > this->m_total_slot_count
        && _tlAssert(
                 "source/phys_mem_new.cpp",
                 314,
                 "GetStuff32(&m_allocated_slot_count) <= GetStuff32(&m_total_slot_count)",
                 "internal error.") )
    {
        __debugbreak();
    }
    slot = 0;
    InterlockedExchange((volatile unsigned int *)&slot, 0);
#else // aislop 
    if (!slot && _tlAssert("source/phys_mem_new.cpp", 306, "slot", ""))
        __debugbreak();

    phys_slot_pool::extra_info *ei = (phys_slot_pool::extra_info *)&slot[(unsigned __int16)this->m_map_key - sizeof(phys_slot_pool::extra_info)];

    if (ei->m_slot_pool_owner != this
        && _tlAssert("source/phys_mem_new.cpp", 309, "GetStuff32(&ei->m_slot_pool_owner) == this_", "internal error."))
    {
        __debugbreak();
    }

    if (InterlockedCompareExchangePointer(
            &ei->m_allocation_owner,
            reinterpret_cast<void *>(kPmmAllocOwnerMagic),
            this) != this
        && _tlAssert("source/phys_mem_new.cpp", 312, "retv", "internal error."))
    {
        __debugbreak();
    }

#if defined(__ANDROID__)
    _InterlockedExchangeAdd(&this->m_allocated_slot_count, 1u);
#else
    _InterlockedExchangeAdd((volatile long *)&this->m_allocated_slot_count, 1u);
#endif
    if (this->m_allocated_slot_count > this->m_total_slot_count
        && _tlAssert(
            "source/phys_mem_new.cpp",
            314,
            "GetStuff32(&m_allocated_slot_count) <= GetStuff32(&m_total_slot_count)",
            "internal error."))
    {
        __debugbreak();
    }
#endif
}

void __thiscall phys_slot_pool::extra_info_free(unsigned __int8 *slot)
{
#if 0
    unsigned __int8 *v3; // ebx
    unsigned int m_map_key; // edi
    volatile unsigned int Target; // [esp+Ch] [ebp-4h] BYREF

    Target = 0;
    InterlockedExchange(&Target, 0);
    v3 = slot;
    m_map_key = this->m_map_key;
    if ( !slot && _tlAssert("source/phys_mem_new.cpp", 323, "slot", "") )
        __debugbreak();
    memset(v3, 0xFFu, (unsigned __int16)m_map_key - 8);
    if ( *(phys_slot_pool **)&v3[(unsigned __int16)m_map_key - 8] != this
        && _tlAssert(
                 "source/phys_mem_new.cpp",
                 329,
                 "GetStuff32(&ei->m_slot_pool_owner) == this_",
                 "attempting to free a slot that was not allocated from this pool.") )
    {
        __debugbreak();
    }
    if ( _InterlockedCompareExchange(
                 (volatile unsigned __int32 *)&v3[(unsigned __int16)m_map_key - 4],
                 (signed __int32)this,
                 -19088744) != -19088744
        && _tlAssert(
                 "source/phys_mem_new.cpp",
                 332,
                 "retv",
                 "incorrect owner attempting to free a slot, possibly trying to free a slot more than once.") )
    {
        __debugbreak();
    }
    _InterlockedExchangeAdd(&this->m_allocated_slot_count, 0xFFFFFFFF);
    if ( this->m_allocated_slot_count > this->m_total_slot_count
        && _tlAssert(
                 "source/phys_mem_new.cpp",
                 334,
                 "GetStuff32(&m_allocated_slot_count) <= GetStuff32(&m_total_slot_count)",
                 "internal error.") )
    {
        __debugbreak();
    }
    if ( this->m_allocated_slot_count < 0
        && _tlAssert("source/phys_mem_new.cpp", 335, "GetStuff32(&m_allocated_slot_count) >= 0", "internal error.") )
    {
        __debugbreak();
    }
    slot = 0;
    InterlockedExchange((volatile unsigned int *)&slot, 0);
#else
    if (!slot && _tlAssert("source/phys_mem_new.cpp", 323, "slot", ""))
        __debugbreak();

    phys_slot_pool::extra_info *ei = (phys_slot_pool::extra_info *)&slot[(unsigned __int16)this->m_map_key - sizeof(phys_slot_pool::extra_info)];

    memset(slot, 0xFFu, (unsigned __int16)this->m_map_key - sizeof(phys_slot_pool::extra_info));

    if (ei->m_slot_pool_owner != this
        && _tlAssert(
            "source/phys_mem_new.cpp",
            329,
            "GetStuff32(&ei->m_slot_pool_owner) == this_",
            "attempting to free a slot that was not allocated from this pool."))
    {
        __debugbreak();
    }

    if (InterlockedCompareExchangePointer(
            &ei->m_allocation_owner,
            this,
            reinterpret_cast<void *>(kPmmAllocOwnerMagic)) != reinterpret_cast<void *>(kPmmAllocOwnerMagic)
        && _tlAssert(
            "source/phys_mem_new.cpp",
            332,
            "retv",
            "incorrect owner attempting to free a slot, possibly trying to free a slot more than once."))
    {
        __debugbreak();
    }

#if defined(__ANDROID__)
    _InterlockedExchangeAdd(&this->m_allocated_slot_count, ~0u);
#else
    _InterlockedExchangeAdd((volatile long *)&this->m_allocated_slot_count, (unsigned long)-1);
#endif

    if (this->m_allocated_slot_count > this->m_total_slot_count
        && _tlAssert(
            "source/phys_mem_new.cpp",
            334,
            "GetStuff32(&m_allocated_slot_count) <= GetStuff32(&m_total_slot_count)",
            "internal error."))
    {
        __debugbreak();
    }
    if ((int)this->m_allocated_slot_count < 0
        && _tlAssert("source/phys_mem_new.cpp", 335, "GetStuff32(&m_allocated_slot_count) >= 0", "internal error."))
    {
        __debugbreak();
    }
#endif
}

void __thiscall phys_slot_pool::validate_slot(char *slot)
{
#if 0
    char *v3; // edi
    volatile unsigned int Target; // [esp+8h] [ebp-4h] BYREF

    Target = 0;
    InterlockedExchange(&Target, 0);
    v3 = &slot[(unsigned __int16)this->m_map_key - 8];
    if ( *(phys_slot_pool **)v3 != this
        && _tlAssert(
                 "source/phys_mem_new.cpp",
                 346,
                 "GetStuff32(&ei->m_slot_pool_owner) == this_",
                 "slot does not belong to this slot_pool.") )
    {
        __debugbreak();
    }
    if ( *((unsigned int *)v3 + 1) != -19088744 )
    {
        if ( _tlAssert(
                     "source/phys_mem_new.cpp",
                     347,
                     "GetStuff32(&ei->m_allocation_owner) == allocation_owner",
                     "slot does not belong to this allocation_owner.") )
        {
            __debugbreak();
        }
    }
#else // aislop
    phys_slot_pool::extra_info *ei = (phys_slot_pool::extra_info *)&slot[(unsigned __int16)this->m_map_key - sizeof(phys_slot_pool::extra_info)];

    if (ei->m_slot_pool_owner != this
        && _tlAssert(
            "source/phys_mem_new.cpp",
            346,
            "GetStuff32(&ei->m_slot_pool_owner) == this_",
            "slot does not belong to this slot_pool."))
    {
        __debugbreak();
    }
    if (ei->m_allocation_owner != reinterpret_cast<void *>(kPmmAllocOwnerMagic)
        && _tlAssert(
            "source/phys_mem_new.cpp",
            347,
            "GetStuff32(&ei->m_allocation_owner) == allocation_owner",
            "slot does not belong to this allocation_owner."))
    {
        __debugbreak();
    }
#endif
}

void phys_slot_pool::free_slot(unsigned __int8 *slot)
{
#if 0
    unsigned __int8 *v3; // edi
    signed __int64 m_first_free_slot; // [esp-Ch] [ebp-1Ch]
    tagged_void_pointer_t first_slot; // [esp+0h] [ebp-10h] BYREF
    //void *v7; // [esp+8h] [ebp-8h]
    //void *retaddr; // [esp+10h] [ebp+0h]

    //first_slot.m_tag = a2;
    //v7 = retaddr;
    v3 = slot;
    if ( !slot && _tlAssert("source/phys_mem_new.cpp", 445, "slot", "no support support for freeing NULL slots.") )
        __debugbreak();
    phys_slot_pool::extra_info_free(slot);
    while ( 1 )
    {
        m_first_free_slot = (signed __int64)this->m_first_free_slot;
        *(unsigned int *)v3 = this->m_first_free_slot.m_ptr;
        first_slot.m_ptr = 0;
        InterlockedExchange((volatile int *)&first_slot, 0);
        if ( _InterlockedCompareExchange64(
                     (volatile signed __int64 *)this,
                     __SPAIR64__(HIDWORD(m_first_free_slot) + 1, (unsigned int)v3),
                     m_first_free_slot) == m_first_free_slot )
            break;
        v3 = slot;
    }

    if (!slot)
    {
        if (_tlAssert("source/phys_mem_new.cpp",
            445,
            "slot",
            "no support support for freeing NULL slots."))
        {
            __debugbreak();
        }
    }
#else
    signed __int64 cur, next;

    if (!slot && _tlAssert("source/phys_mem_new.cpp", 445, "slot", "no support for freeing NULL slots."))
        __debugbreak();

    this->extra_info_free(slot);

    while (1)
    {
        cur = *(volatile signed __int64 *)&this->m_first_free_slot;
        *(unsigned int *)slot = LODWORD(cur); // link slot->next = current head bits
        next = __SPAIR64__(HIDWORD(cur) + 1, pmm_ptr_to_bits(slot));
        if (_InterlockedCompareExchange64(
            (volatile signed __int64 *)&this->m_first_free_slot,
            next,
            cur) == cur)
            break;
    }
#endif
}

char * phys_slot_pool::allocate_slot()
{
#if 0
    char **m_ptr; // edi
    char *v3; // esi
    char *v5; // esi
    char *v6; // [esp-8h] [ebp-18h]
    int v7; // [esp-4h] [ebp-14h] BYREF
    tagged_void_pointer_t first_slot; // [esp+0h] [ebp-10h]
    void *v9; // [esp+8h] [ebp-8h]
    void *retaddr; // [esp+10h] [ebp+0h]

    first_slot.m_tag = a2;
    v9 = retaddr;
    m_ptr = (char **)this;
    for ( first_slot.m_ptr = this; ; m_ptr = (char **)first_slot.m_ptr )
    {
        v3 = *m_ptr;
        v6 = m_ptr[1];
        if ( !*m_ptr )
            break;
        v7 = 0;
        InterlockedExchange(&v7, 0);
        if ( _InterlockedCompareExchange64(
                     (volatile signed __int64 *)m_ptr,
                     __SPAIR64__((unsigned int)(v6 + 1), *(unsigned int *)v3),
                     __SPAIR64__((unsigned int)v6, (unsigned int)v3)) == __PAIR64__((unsigned int)v6, (unsigned int)v3) )
        {
            phys_slot_pool::extra_info_allocate((phys_slot_pool *)first_slot.m_ptr, v3);
            return v3;
        }
    }
    v5 = (char *)phys_memory_manager::allocate(
                                 g_phys_memory_manager,
                                 (unsigned __int16)m_ptr[2],
                                 (unsigned int)m_ptr[2] >> 16);
    phys_slot_pool::extra_info_init((phys_slot_pool *)m_ptr, v5);
    return v5;
#else // aislop
    signed __int64 cur, next;
    char *v3;

    while (1)
    {
        cur = *(volatile signed __int64 *)&this->m_first_free_slot;
        v3 = static_cast<char *>(pmm_bits_to_ptr(LODWORD(cur)));
        if (!v3)
            break; // free list empty, fall through to fresh alloc

        next = __SPAIR64__(HIDWORD(cur) + 1, *(unsigned int *)v3);
        if (_InterlockedCompareExchange64(
            (volatile signed __int64 *)&this->m_first_free_slot,
            next,
            cur) == cur)
        {
            this->extra_info_allocate(v3);
            return v3;
        }
    }

    char *v5 = static_cast<char *>(g_phys_memory_manager->allocate(
        (unsigned __int16)this->m_map_key,
        (unsigned int)this->m_map_key >> 16));
    this->extra_info_init(v5);
    return v5;
#endif
}

