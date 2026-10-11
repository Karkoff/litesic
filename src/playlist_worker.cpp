#include "playlist_worker.h"
#include "m3u.h"

bool PlaylistWorker::Start(HWND destination) {
    if (initialized_) return true;
    InitializeCriticalSection(&lock_);
    initialized_ = true; destination_ = destination;
    stop_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    ready_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    space_ = CreateSemaphoreW(nullptr, 64, 64, nullptr);
    if (stop_ && ready_ && space_) thread_ = CreateThread(nullptr, 0, Entry, this, 0, nullptr);
    if (!thread_) { Stop(); return false; }
    return true;
}
void PlaylistWorker::Stop() {
    if (!initialized_) return;
    if (stop_) SetEvent(stop_);
    if (thread_) { WaitForSingleObject(thread_, INFINITE); CloseHandle(thread_); }
    if (stop_) CloseHandle(stop_);
    if (ready_) CloseHandle(ready_);
    if (space_) CloseHandle(space_);
    jobs_.clear(); results_.clear(); pending_ = 0;
    DeleteCriticalSection(&lock_);
    initialized_ = false; thread_ = stop_ = ready_ = space_ = nullptr;
}
bool PlaylistWorker::Submit(PlaylistJob job) {
    if (!thread_) return false;
    job.generation = Generation();
    EnterCriticalSection(&lock_);
    jobs_.push_back(std::move(job));
    InterlockedIncrement(&pending_);
    SetEvent(ready_);
    LeaveCriticalSection(&lock_);
    return true;
}
LONG PlaylistWorker::NewGeneration() {
    LONG generation = InterlockedIncrement(&generation_);
    if (initialized_) {
        EnterCriticalSection(&lock_);
        InterlockedExchangeAdd(&pending_, -static_cast<LONG>(jobs_.size()));
        jobs_.clear(); ResetEvent(ready_);
        LeaveCriticalSection(&lock_);
    }
    return generation;
}
bool PlaylistWorker::Cancelled(LONG generation) const {
    return generation != Generation() || WaitForSingleObject(stop_, 0) == WAIT_OBJECT_0;
}
bool PlaylistWorker::Publish(PlaylistResult result) {
    if (Cancelled(result.generation)) return false;
    HANDLE wait[] = {stop_, space_};
    if (WaitForMultipleObjects(2, wait, FALSE, INFINITE) != WAIT_OBJECT_0 + 1) return false;
    if (Cancelled(result.generation)) { ReleaseSemaphore(space_, 1, nullptr); return false; }
    EnterCriticalSection(&lock_);
    results_.push_back(std::move(result));
    LeaveCriticalSection(&lock_);
    // Message carries no heap pointer. Draining/shutdown owns all result memory.
    return PostMessageW(destination_, WM_PLAYLIST_RESULTS, 0, 0) != FALSE;
}
std::vector<PlaylistResult> PlaylistWorker::Drain() {
    std::vector<PlaylistResult> drained;
    if (!initialized_) return drained;
    EnterCriticalSection(&lock_);
    while (!results_.empty()) {
        drained.push_back(std::move(results_.front())); results_.pop_front();
    }
    LeaveCriticalSection(&lock_);
    if (!drained.empty()) ReleaseSemaphore(space_, static_cast<LONG>(drained.size()), nullptr);
    return drained;
}
DWORD WINAPI PlaylistWorker::Entry(void* self) {
    static_cast<PlaylistWorker*>(self)->Run(); return 0;
}
void PlaylistWorker::ScanFolder(const PlaylistJob& job) {
    std::vector<std::wstring> folders = {job.path};
    PlaylistResult result; result.kind = PlaylistJob::Scan; result.generation = job.generation;
    while (!folders.empty() && !Cancelled(job.generation)) {
        std::wstring folder = std::move(folders.back()); folders.pop_back();
        if (!folder.empty() && folder.back() != L'\\' && folder.back() != L'/') folder += L'\\';
        WIN32_FIND_DATAW data{};
        HANDLE find = FindFirstFileW((folder + L"*").c_str(), &data);
        if (find == INVALID_HANDLE_VALUE) continue;
        do {
            if (Cancelled(job.generation)) break;
            if (data.cFileName[0] == L'.' && (data.cFileName[1] == 0 ||
                (data.cFileName[1] == L'.' && data.cFileName[2] == 0))) continue;
            std::wstring path = folder + data.cFileName;
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Do not follow junctions/symlinks: recursive cycles are possible.
                if (!(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) folders.push_back(path);
            } else if (IsSupportedAudioFile(path.c_str())) {
                Track track; track.path = path;
                result.tracks.push_back(std::move(track));
                if (result.tracks.size() == 32) {
                    if (!Publish(std::move(result))) { FindClose(find); return; }
                    result = {}; result.kind = PlaylistJob::Scan; result.generation = job.generation;
                }
            }
        } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    if (!result.tracks.empty()) Publish(std::move(result));
}
void PlaylistWorker::Run() {
    HANDLE wait[] = {stop_, ready_};
    while (WaitForMultipleObjects(2, wait, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
        PlaylistJob job;
        EnterCriticalSection(&lock_);
        bool haveJob = !jobs_.empty();
        if (haveJob) { job = std::move(jobs_.front()); jobs_.pop_front(); }
        if (jobs_.empty()) ResetEvent(ready_);
        LeaveCriticalSection(&lock_);
        if (!haveJob) continue;
        if (!Cancelled(job.generation)) {
            if (job.kind == PlaylistJob::Scan) ScanFolder(job);
            else {
                PlaylistResult result; result.kind = job.kind;
                result.id = job.id; result.generation = job.generation;
                if (job.kind == PlaylistJob::Metadata) result.metadata = ReadTrackMetadata(job.path);
                else if (job.kind == PlaylistJob::Open) LoadM3U(job.path, result.tracks, result.error);
                else SaveM3U(job.path, job.tracks, result.error);
                Publish(std::move(result));
            }
        }
        InterlockedDecrement(&pending_);
        PostMessageW(destination_, WM_PLAYLIST_RESULTS, 0, 0);
    }
}
