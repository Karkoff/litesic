#include "playlist_worker.h"
#include "check.h"
#include "m3u.h"
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
static void notifications_coalesce_until_drain(){
    MSG message{};while(PeekMessageW(&message,window,WM_PLAYLIST_RESULTS,WM_PLAYLIST_RESULTS,PM_REMOVE)){}
    PlaylistWorker worker;CHECK(worker.Start(window));
    for(int batch=0;batch<2;++batch){
        for(int i=0;i<16;++i){PlaylistJob job;job.id=i;job.path=L"tests\\fixtures\\native-44100.wav";CHECK(worker.Submit(job));}
        DWORD deadline=GetTickCount()+10000;
        while(worker.Busy()){CHECK(static_cast<LONG>(GetTickCount()-deadline)<0);Sleep(1);}
        // Completion may race the Busy flag; allow its coalesced wakeup to finish.
        Sleep(10);int notifications=0;
        while(PeekMessageW(&message,window,WM_PLAYLIST_RESULTS,WM_PLAYLIST_RESULTS,PM_REMOVE))++notifications;
        CHECK(notifications==1);CHECK(worker.Drain().size()==16);
    }
    worker.Stop();
}
static void accepted_exports_survive_clear_and_shutdown(){
    for(bool clear:{false,true}){
        PlaylistWorker worker;CHECK(worker.Start(window));
        const auto path=FullPath(clear?L"build\\export-after-clear.m3u":L"build\\export-at-shutdown.m3u8");
        DeleteFileW(path.c_str());
        for(int i=0;i<200;++i){PlaylistJob metadata;metadata.path=L"tests\\fixtures\\native-44100.wav";CHECK(worker.Submit(metadata));}
        Track track;track.path=FullPath(L"tests\\fixtures\\tagged.mp3");track.favorite=true;
        PlaylistJob save;save.kind=PlaylistJob::Save;save.path=path;save.tracks={track};CHECK(worker.Submit(save));
        if(clear){
            worker.NewGeneration();auto results=Await(worker);int saves=0;
            for(const auto& result:results)if(result.kind==PlaylistJob::Save){++saves;CHECK(result.error.empty());}
            CHECK(saves==1);
        }
        worker.Stop();
        std::vector<Track> loaded;std::wstring error;
        CHECK(LoadM3U(path,loaded,error));CHECK(loaded.size()==1);
        CHECK(loaded[0].path==track.path&&loaded[0].favorite);
        CHECK(DeleteFileW(path.c_str()));
    }
}
int main(){window=CreateWindowExW(0,L"STATIC",L"worker-test",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
    int cases=0;CASE(actual_metadata_job);CASE(recursive_filter);CASE(generation_cancel_and_shutdown);CASE(notifications_coalesce_until_drain);CASE(accepted_exports_survive_clear_and_shutdown);DestroyWindow(window);std::printf("%d worker cases\n",cases);}
