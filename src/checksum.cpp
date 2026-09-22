// POSIX cksum (CRC-32 plus byte length), bundled so firmware utilities are
// irrelevant. Compatible with nonempty manifests made by previous releases.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sys/stat.h>
static uint32_t byte(uint32_t crc, unsigned char b) {
  crc ^= uint32_t(b) << 24;
  for (int i=0; i<8; ++i)
    crc = (crc & 0x80000000U) ? (crc << 1) ^ 0x04c11db7U : crc << 1;
  return crc;
}
static uint32_t finish(uint32_t crc, uint64_t length) {
  while (length) { crc=byte(crc,static_cast<unsigned char>(length)); length >>= 8; }
  return ~crc;
}
int main(int argc, char** argv) {
  if (argc != 2) { std::fprintf(stderr,"usage: audiocast-cksum FILE | --self-test\n"); return 2; }
  if (!std::strcmp(argv[1],"--self-test")) {
    uint32_t crc=0;
    const unsigned char abc[]={'a','b','c'};
    for (unsigned char b : abc) crc=byte(crc,b);
    return finish(0,0)==4294967295U && finish(crc,3)==1219131554U ? 0 : 1;
  }
  FILE* f=std::fopen(argv[1],"rb");
  if (!f) { std::perror("checksum open"); return 1; }
  struct stat info{};
  if (fstat(fileno(f),&info) || !S_ISREG(info.st_mode)) {
    std::fprintf(stderr,"checksum requires a readable regular file\n"); std::fclose(f); return 1;
  }
  uint32_t crc=0;
  uint64_t length=0;
  unsigned char buffer[4096];
  size_t count;
  while ((count=std::fread(buffer,1,sizeof(buffer),f))) {
    for (size_t i=0; i<count; ++i) crc=byte(crc,buffer[i]);
    length+=count;
  }
  bool failed=std::ferror(f);
  if (std::fclose(f)) failed=true;
  if (failed) { std::fprintf(stderr,"checksum read failed\n"); return 1; }
  std::printf("%u %llu\n",finish(crc,length),static_cast<unsigned long long>(length));
  return 0;
}
