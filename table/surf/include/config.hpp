#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include <string.h>
#include <string>
namespace surf {

using level_t = uint32_t;
using position_t = uint32_t;
static const position_t kMaxPos = UINT32_MAX;

using label_t = uint8_t;
static const position_t kFanout = 256;

using word_t = uint64_t;
static const unsigned kWordSize = 64;
static const unsigned kValueSize = 64;
static const word_t kMsbMask = 0x8000000000000000;
static const word_t kOneMask = 0xFFFFFFFFFFFFFFFF;

static const bool kIncludeDense = true;
//static const uint32_t kSparseDenseRatio = 64;
static const uint32_t kSparseDenseRatio = 16;
static const label_t kTerminator = 255;

static const int kHashShift = 7;

static const int kCouldBePositive = 2018; // used in suffix comparison

enum SuffixType {
    kNone = 0,
    kHash = 1,
    kReal = 2,
    kMixed = 3
};
void align(char*& ptr);
void sizeAlign(position_t& size);
void sizeAlign(uint64_t& size);
std::string uint64ToString(const uint64_t word);
uint64_t stringToUint64(const std::string& str_word);
} // namespace surf

#endif // CONFIG_H_
