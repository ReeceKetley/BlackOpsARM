# SDK470 live tablet CPU thread usage

Read-only sampling of the existing foreground interrogation-room process,
PID28098, engine BuildId `b1a729750397ae1b463e0dfab606091d97232638`.
The screen was awake, all eight CPUs online, and the same process remained alive.
No binary, assets, scheduling settings or worker count was changed.

Five snapshots span73.02 seconds. CPU time comes from each thread's Linux
`schedstat` runtime counter; independent utime+stime counters agree to within
0.1 percentage points for the major threads.100% means one CPU's worth of work.
The interval uses host monotonic time at read midpoints. Snapshot transport/read
durations were13.07–13.35 seconds; stable ordering and similar duration limit
timing skew, but these remain observational averages rather than a controlled
gameplay benchmark. Last-CPU samples show placement, not complete CPU residency
or simultaneous utilization of every core.

| Thread | CPU use | Allowed CPUs | CPUs observed |
| --- | ---: | --- | --- |
| SDL audio callback SDLAudioP2 |56.6%|0–7|4,5,6|
| bo1-render |32.7%|0–7|4,6,7|
| Engine main SDLThread28186 |31.7%|0–7|4,6,7|
| Job worker1 |23.3%|0–7|4,5,6,7|
| Job worker2 |23.2%|0–7|4,6,7|
| SERVER |23.1%|0–7|4,6,7|
| Additional SDLThread28243 |14.8%|0–7|4,7|
| Android AudioTrack |7.3%|0–7|0,1,2,3|

Total across54 threads is227.7%, approximately2.28 CPUs' worth of work. Every
sampled thread has affinity0–7; the union of observed CPU placements covers all
eight CPUs. The heavy threads mainly ran on CPUs4–7. No thread averages close to
one fully occupied core during this sample. These averages cannot rule out
short bursts or a serial stage limiting frame completion.

Sleeping/waiting states were sampled on several busy threads, including a
futex wait on the render thread. The main thread accrued about6.8 seconds of
run-queue wait during the interval; each job worker about4.1–4.3 seconds.
Run-queue wait is time ready to run without CPU service, not GPU wait or total
blocked time. A sampled wchan cannot establish the dominant frame bottleneck.
The existing GPU span measurement includes its defined query interval and is
not GPU busy percentage; further critical-path evidence is needed before
assigning the frame limit to CPU, GPU or synchronization.

Source confirms the production job queue starts two dedicated job workers:
`R_InitWorkerCmds` selects mask12 and the queue also provides the main assist
worker. This matches the Windows oracle's setup. Android queries the online
CPU count and its scheduling provider preserves inherited affinity for an
eight-CPU system. The registered `r_smp_worker_threads` variable alone does not
change this job-worker creation path. Increasing workers requires proving queue,
thread-context, renderer ownership and available parallel work first.

The immediate performance leads are audio callback cost, renderer/main waits
and the two-worker pipeline. Filling every core is not itself a performance
goal: frame time and playable gameplay are the relevant outcomes. This room
sample does not establish full Zombies workload balance or30FPS feasibility.

Evidence: `build-android-audit/native_registry_closure/sdk470-cpu-sample2/`
contains all five raw thread snapshots, errors and `results.json`. The first
attempt's `/proc/uptime` access denial/parser failure is preserved in
`sdk470-cpu-sample1/`; it is not counted as a successful measurement. The corrected
sampler uses host elapsed time and shell built-in proc reads.
