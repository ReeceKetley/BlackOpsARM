# Native scene and render worker callback audit

2026-10-14. Read-only bounded audit while SDK455 is blocked in frustum setup. No source, provider, asset or device changes were made by this audit. This is source/preprocessing evidence, not runtime execution of every conditional path.

The actual scene/culling/render-worker boundary uses matching canonical types: all19 renderer jq module registrations and queue consumers use int(jqBatch*); Android worker entry uses unsigned(jqWorker*); assist callbacks use existing bool(void*) adapters. The render-thread entry retains void(unsigned int). The29 backend command-table slots (28non-null) and16 tess dispatch slots directly store matching types. Legacy Windows cast branches are excluded from current native preprocessing.

A separate conditional emblem-composition path has one active native signature mismatch. cl_compositing.cpp stores CL_PCCopyImageGenMIPCallback(unsigned int*) through a void(void*) cast in R_AddCmdPCCopyImageGenMIP. The command stores that callback and job pointer, then RB_PCCopyImageGenMIPCmd calls callback(job). The payload is a process-owned CompositeJob in static s_jobs[4], and the canonical callback writes its first four-byte state to COMPLETE. Current Android preprocessing confirms registration and dispatch are active, but tablet execution of this conditional emblem path has not been observed.

The narrow future repair is an exact native void(void*) adapter forwarding to the unchanged canonical helper with static_cast<unsigned int*>(context). It is recorded for future relevant integration and does not block the currently observed Zombies scene path. No adapter or placeholder was added by this audit.

Primary source contracts: src/gfx_d3d/r_workercmds.cpp, src/universal/com_workercmds.cpp, src/gfx_d3d/rb_backend.cpp and rb_backend.h, src/client/cl_compositing.cpp and cl_compositing.h, src/gfx_d3d/r_rendercmds.cpp. Root received the completed renderer_job_signatures audit and retained it in this report.
