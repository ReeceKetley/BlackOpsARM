# Canonical system metadata owner restoration

2026-10-13. Original Sys_GetInfo and Sys_ArchiveInfo were inside the broad legacy
Windows/headless exclusion despite having genuine portable bodies. They are now
single canonical shared owners selected by the full native system-core build;
the historical diagnostic selection retains its previous exclusion.

Sys_GetInfo copies the actual sys_info using native sizeof(SysInfo), preserving
native long-double width rather than copying a Windows-sized record. Sys_ArchiveInfo
uses the existing real Sys_RegisterInfoDvars and archives actual configureGHz,
memory, GPU description and checksum into their canonical Dvars. No replacement
hardware values, early Dvar initialization or readiness flags are introduced.
Normal Windows bodies are unchanged, only their declaration placement differs.

Whole native win_main.cpp compilation passes with the actual full target flags
in native_registry_closure/win_main_metadata_audit1.log. Whole normal Windows
win_main.cpp passes both ABIs in takeover_system_metadata_windows1. Actual
initialized configuration/recommendation execution and the user confirmation
policy remain unverified. This source edit follows accepted full-engine-link391
and needs the next integrated link; it does not prove Com_Init return.
