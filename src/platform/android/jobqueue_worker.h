#pragma once
struct jqWorker;
namespace bo1::android {
void *SpawnJobWorker(jqWorker *worker,unsigned (*function)(jqWorker *));
void JobWorkerReady(jqWorker *worker);
void JoinJobWorker(void *handle);
}
