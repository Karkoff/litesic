#include "tags.h"
#include "check.h"
#include <fstream>
#include <iterator>
#include <cmath>
static std::vector<uint8_t> Bytes(const char* file) {
    std::ifstream input(file,std::ios::binary); CHECK(input.good());
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),{});
}
static void known(const wchar_t* path) {
    auto metadata=ReadTrackMetadata(path);CHECK(metadata.readable);CHECK(metadata.title==L"Canção M5");
    CHECK(metadata.artist==L"LiteSic");CHECK(metadata.duration>0.24 && metadata.duration<0.35);
}
static void mp3_v3(){known(L"tests\\fixtures\\tagged.mp3");}
static void mp3_v4(){known(L"tests\\fixtures\\tagged-v4.mp3");}
static void flac_comments(){known(L"tests\\fixtures\\tagged.flac");}
static void ogg_spanning_pages(){known(L"tests\\fixtures\\tagged.ogg");}
static void id3_v1(){auto m=ReadTrackMetadata(L"tests\\fixtures\\tagged-v1.mp3");CHECK(m.readable);CHECK(m.title==L"Faixa antiga");CHECK(m.artist==L"LiteSic");}
static void wav_native_duration(){auto m=ReadTrackMetadata(L"tests\\fixtures\\native-44100.wav");CHECK(m.readable);CHECK(m.title.empty());CHECK(std::abs(m.duration-2)<0.00001);}
static void missing_invalid(){CHECK(!ReadTrackMetadata(L"tests\\fixtures\\missing.wav").readable);CHECK(!ReadTrackMetadata(L"README.md").readable);}
static void truncation_bounds(){
    for(const char* name:{"tests/fixtures/tagged.mp3","tests/fixtures/tagged.flac","tests/fixtures/tagged.ogg"}) {
        auto data=Bytes(name);
        for(size_t n=0;n<data.size();n+=17) {std::vector<uint8_t> prefix(data.begin(),data.begin()+n);ParseTags(prefix,{});}
    }
    std::vector<uint8_t> invalid={'I','D','3',4,0,0,127,127,127,127};CHECK(ParseTags(invalid,{}).title.empty());
}
int main(){int cases=0;CASE(mp3_v3);CASE(mp3_v4);CASE(flac_comments);CASE(ogg_spanning_pages);
    CASE(id3_v1);CASE(wav_native_duration);CASE(missing_invalid);CASE(truncation_bounds);std::printf("%d tags cases\n",cases);}
