#include "playlist_worker.h"
#include "check.h"
static HWND window;
static std::vector<PlaylistResult> Await(PlaylistWorker& worker){
    std::vector<PlaylistResult> results;DWORD deadline=GetTickCount()+10000;
    do {
        MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        for(auto& r:worker.Drain()) results.push_back(std::move(r));
        if(!worker.Busy()) break;
        MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);
    }while(static_cast<LONG>(GetTickCount()-deadline)<0);
    CHECK(!worker.Busy());for(auto& r:worker.Drain()) results.push_back(std::move(r));return results;
}
static void actual_metadata_job(){PlaylistWorker worker;CHECK(worker.Start(window));PlaylistJob job;job.id=42;job.path=L"tests\\fixtures\\tagged.flac";
    CHECK(worker.Submit(job));auto results=Await(worker);CHECK(results.size()==1);CHECK(results[0].id==42);CHECK(results[0].metadata.title==L"Canção M5");worker.Stop();}
static void recursive_filter(){PlaylistWorker worker;CHECK(worker.Start(window));PlaylistJob job;job.kind=PlaylistJob::Scan;job.path=L"tests\\fixtures";
    CHECK(worker.Submit(job));auto results=Await(worker);size_t count=0;for(auto& r:results)count+=r.tracks.size();CHECK(count==7);worker.Stop();}
static void generation_cancel_and_shutdown(){PlaylistWorker worker;CHECK(worker.Start(window));
    for(int i=0;i<200;++i){PlaylistJob job;job.id=i;job.path=L"tests\\fixtures\\native-44100.wav";CHECK(worker.Submit(job));}
    LONG generation=worker.NewGeneration();auto results=Await(worker);for(auto& r:results)CHECK(r.generation!=generation);
    PlaylistJob missing;missing.path=L"missing.wav";CHECK(worker.Submit(missing));results=Await(worker);CHECK(results.size()==1);CHECK(!results[0].metadata.readable);
    worker.Stop();CHECK(!worker.Submit(missing));CHECK(worker.Start(window));worker.Stop();}
int main(){window=CreateWindowExW(0,L"STATIC",L"worker-test",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
    int cases=0;CASE(actual_metadata_job);CASE(recursive_filter);CASE(generation_cancel_and_shutdown);DestroyWindow(window);std::printf("%d worker cases\n",cases);}
