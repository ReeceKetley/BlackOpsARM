# Native SP process-resource measurement

2026-10-13. SP measurement port in progress; genuine Com_Init remains incomplete.

process_resources.cpp/h implements actual procfs observations for this process:
VmRSS, VmHWM, RssAnon and numeric /proc/self/fd entries excluding the query's
directory descriptor. Owned descriptors close before return. Memory values have
validated kB units and overflow checks. Oversized/missing/malformed status fields,
OS errors and invalid output report failure/errno without publishing partial output.
These are best-effort sequential observations, not an atomic process snapshot.

Canonical g_sp_measure.cpp resource reporting now consumes this provider on
Android, explicitly naming rss_kb/anonymous_rss_kb/peak_rss_kb/open_fds. Failure
emits resources_unavailable with errno. It does not mislabel anonymous resident
memory as Windows private committed bytes, nor file descriptors as Windows handles.
The original Windows counter/report branch is unchanged.

Evidence under build-android-audit/native_registry_closure:

- process-resources-build1.log: native production service and focused test compile
  and link under UBSan in bo1_native_process_resources_tests.
- process-resources-run1.log: actual HONOR execution exit0, pipe descriptor counts
  3->5->3; touched16MiB RSS2486272->19238912 bytes; anonymous729088->17428480;
  peak19238912. Invalid-null argument reports EINVAL. RSS deltas are observations,
  not strict assertions against kernel approximation. Failure-field parsing and
  concurrent descriptor churn were not injected/tested.
- Only changed450024-byte test executable pushed; assets/SDL/other probes reused.
- full-engine-link270.log compiles the provider in entry-state; full engine still
  fails genuine unresolved links.
- takeover_sp_measure_resource_branch_windows1: entire normal g_sp_measure.cpp
  passes x86/x64 exit0 after the resource branch edit.

The measurement TU remains absent from the full native source list until its
remaining timing/thread/process CPU/x86 zone-counter/formatting paths are adapted.
The new reporting branch has not compiled or run as a whole Android TU yet;
provider fixture is narrower than SP resource event execution. Do not claim it
resolves G_SP_MeasureEnabled or initializes gameplay. Next finish those native
measurement boundaries and integrate the actual source. Full Com_Init/Com_Frame
and native workload/owner lifecycles remain incomplete.
