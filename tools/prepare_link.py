"""Initialize unused Link buffer placeholders before optimized constructor copies.

Runtime commit/encoding still fills metadata normally. The pinned SDK otherwise
copies uninitialized IDs/bytes while constructing its queues/processors.
"""
from pathlib import Path
import sys

root=Path(sys.argv[1])/"include/ableton/link_audio"
changes={
    "Buffer.hpp":{
        "  uint32_t mSampleRate;":"  uint32_t mSampleRate = 0;",
        "  uint32_t mNumChannels;":"  uint32_t mNumChannels = 0;",
        "  uint32_t mNumFrames;":"  uint32_t mNumFrames = 0;",
        "  uint64_t mCount;":"  uint64_t mCount = 0;",
        "  Id mSessionId;":"  Id mSessionId{};",
    },
    "AudioBuffer.hpp":{
        "  Id channelId;":"  Id channelId{};",
        "  Id sessionId;":"  Id sessionId{};",
        "  Codec codec;":"  Codec codec = kInvalid;",
        "  uint32_t sampleRate;":"  uint32_t sampleRate = 0;",
        "  uint8_t numChannels;":"  uint8_t numChannels = 0;",
        "  uint16_t numBytes;":"  uint16_t numBytes = 0;",
        "  Bytes bytes;":"  Bytes bytes{};",
    },
}
prepared={}
for name,replacements in changes.items():
    path=root/name;text=path.read_text()
    for old,new in replacements.items():
        if text.count(new)==1:continue
        if text.count(old)!=1:raise SystemExit("Pinned Link header differs: "+str(path)+": "+old)
        text=text.replace(old,new)
    prepared[path]=text
for path,text in prepared.items():path.write_text(text)
print("Initialized Link placeholder metadata; no packet or commit format changes.")
