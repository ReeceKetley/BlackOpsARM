#pragma once

struct TraceCheckCount // sizeof=0x8
{                                                                             // XREF: TraceThreadInfo/r
    int global;
    int *partitions;
};

struct TraceThreadInfo // sizeof=0x14
{                                                                             // XREF: .data:TraceThreadInfo * g_traceThreadInfo/r
    TraceCheckCount checkcount;
    struct cbrush_t *box_brush;                                // XREF: CM_Trace(trace_t *,float const * const,float const * const,float const * const,float const * const,uint,int,col_context_t &)+66E/r
    // CM_Trace(trace_t *,float const * const,float const * const,float const * const,float const * const,uint,int,col_context_t &)+94C/r ...
    struct cmodel_t *box_model;
    struct PhysGeomList **geoms;
};

