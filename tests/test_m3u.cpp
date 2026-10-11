#include "m3u.h"
#include "check.h"
#include <windows.h>
#include <cmath>
#include <fstream>
static void relative_absolute_extinf() {
    std::vector<Track> tracks;
    CHECK(ParseM3U(L"\ufeff#EXTM3U\r\n#EXTINF:12.5,Artista — Canção\r\n..\\audio\\a.mp3\r\nD:\\music\\b.flac\n",
                   L"C:\\lists\\mix.m3u",tracks));
    CHECK(tracks.size()==2);CHECK(tracks[0].path==L"C:\\audio\\a.mp3");
    CHECK(tracks[0].displayTitle==L"Artista — Canção");CHECK(tracks[0].duration==12.5);
    CHECK(tracks[1].path==L"D:\\music\\b.flac");CHECK(tracks[1].duration==-1);
}
static void plain_and_unsupported() {
    std::vector<Track> tracks;ParseM3U(L"# comment\n\na.wav\nhttps://host/a.mp3\nnotes.txt\n\"b.ogg\"\n",L"C:\\list.m3u",tracks);
    CHECK(tracks.size()==2);CHECK(tracks[0].displayTitle==L"a");CHECK(tracks[1].path==L"C:\\b.ogg");
}
static void malformed_extinf() {
    std::vector<Track> tracks;ParseM3U(L"#EXTINF:nan,Title\na.mp3\n#EXTINF:bad\nb.wav\n",L"C:\\x.m3u",tracks);
    CHECK(tracks.size()==2);CHECK(tracks[0].duration==-1);CHECK(tracks[1].displayTitle==L"b");
}
static void unicode_roundtrip() {
    Track t;t.path=L"C:\\Música\\Canção.wav";t.displayTitle=L"日本語 — canção";t.duration=2.25;
    std::vector<Track> result;std::wstring decoded;
    CHECK(DecodeText(EncodeUtf8(WriteM3U({t})),decoded));CHECK(ParseM3U(decoded,L"D:\\mix.m3u",result));
    CHECK(result.size()==1);CHECK(result[0].path==t.path);CHECK(result[0].displayTitle==t.displayTitle);CHECK(result[0].duration==2.25);
}
static void encoding_errors_and_legacy() {
    std::wstring text;CHECK(!DecodeText(std::string("\xff"),text));CHECK(DecodeText(std::string("\xe7"),text,true));CHECK(text==L"ç");
    CHECK(DecodeText("\xef\xbb\xbfhello",text));CHECK(text==L"hello");
}
static void actual_file_roundtrip_and_errors() {
    CreateDirectoryW(L"build\\test-data",nullptr);
    Track t;t.path=FullPath(L"tests\\fixtures\\native-44100.wav");t.displayTitle=L"Fixture — ç";t.duration=2;
    std::wstring error;std::vector<Track> loaded;
    CHECK(SaveM3U(L"build\\test-data\\roundtrip.m3u8",{t},error));CHECK(error.empty());
    std::ifstream saved("build/test-data/roundtrip.m3u8",std::ios::binary);
    CHECK(saved.get()=='#'); // External-player compatibility: no leading BOM.
    CHECK(LoadM3U(L"build\\test-data\\roundtrip.m3u8",loaded,error));CHECK(loaded.size()==1);CHECK(loaded[0].path==t.path);
    CHECK(!LoadM3U(L"build\\test-data\\absent.m3u",loaded,error));CHECK(!error.empty());
    CHECK(!SaveM3U(L"build\\test-data\\absent-folder\\x.m3u",{t},error));CHECK(!error.empty());
}
int main(){int cases=0;CASE(relative_absolute_extinf);CASE(plain_and_unsupported);CASE(malformed_extinf);
    CASE(unicode_roundtrip);CASE(encoding_errors_and_legacy);CASE(actual_file_roundtrip_and_errors);std::printf("%d m3u cases\n",cases);}
