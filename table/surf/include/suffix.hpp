#ifndef SUFFIX_H_
#define SUFFIX_H_

#include "bitvector.hpp"

#include <assert.h>

#include <vector>

#include "config.hpp"
#include "hash.hpp"

namespace surf {

// Max suffix_len_ = 64 bits
// For kReal suffixes, if the stored key is not long enough to provide
// suffix_len_ suffix bits, its suffix field is cleared (i.e., all 0's)
// to indicate that there is no suffix info associated with the key.
class BitvectorSuffix : public Bitvector {
public:
    BitvectorSuffix() : type_(kNone), hash_suffix_len_(0), real_suffix_len_(0) {};

    BitvectorSuffix(const SuffixType type,
                    const level_t hash_suffix_len, const level_t real_suffix_len,
                    const std::vector<std::vector<word_t> >& bitvector_per_level,
                    const std::vector<position_t>& num_bits_per_level,
                    const level_t start_level = 0,
                    level_t end_level = 0/* non-inclusive */)
	: Bitvector(bitvector_per_level, num_bits_per_level, start_level, end_level) {
	assert((hash_suffix_len + real_suffix_len) <= kWordSize);
	type_ = type;
	hash_suffix_len_ = hash_suffix_len;
        real_suffix_len_ = real_suffix_len;
    }

    static word_t constructHashSuffix(const std::string& key, const level_t len) {
	word_t suffix = suffixHash(key);
	suffix <<= (kWordSize - len - kHashShift);
	suffix >>= (kWordSize - len);
	return suffix;
    }

    static word_t constructRealSuffix(const std::string& key,
				      const level_t level, const level_t len) {
	if (key.length() < level || ((key.length() - level) * 8) < len)
	    return 0;
	word_t suffix = 0;
	level_t num_complete_bytes = len / 8;
	if (num_complete_bytes > 0) {
	    suffix += (word_t)(label_t)key[level];
	    for (position_t i = 1; i < num_complete_bytes; i++) {
		suffix <<= 8;
		suffix += (word_t)(uint8_t)key[level + i];
	    }
	}
	level_t offset = len % 8;
	if (offset > 0) {
	    suffix <<= offset;
	    word_t remaining_bits = 0;
	    remaining_bits = (word_t)(uint8_t)key[level + num_complete_bytes];
	    remaining_bits >>= (8 - offset);
	    suffix += remaining_bits;
	}
	return suffix;
    }

    static word_t constructMixedSuffix(const std::string& key, const level_t hash_len,
				       const level_t real_level, const level_t real_len) {
        word_t hash_suffix = constructHashSuffix(key, hash_len);
        word_t real_suffix = constructRealSuffix(key, real_level, real_len);
        word_t suffix = hash_suffix;
        suffix <<= real_len;
        suffix |= real_suffix;
        return suffix;
    }

    static word_t constructSuffix(const SuffixType type, const std::string& key,
                                  const level_t hash_len,
                                  const level_t real_level, const level_t real_len) {
	switch (type) {
	case kHash:
	    return constructHashSuffix(key, hash_len);
	case kReal:
	    return constructRealSuffix(key, real_level, real_len);
        case kMixed:
            return constructMixedSuffix(key, hash_len, real_level, real_len);
	default:
	    return 0;
        }
    }

    static word_t extractHashSuffix(const word_t suffix, const level_t real_suffix_len) {
        return (suffix >> real_suffix_len);
    }

    static word_t extractRealSuffix(const word_t suffix, const level_t real_suffix_len) {
        word_t real_suffix_mask = 1;
        if(real_suffix_len == 64) {
            real_suffix_mask = 0xffffffffffffffff;
        } else {
            real_suffix_mask <<= real_suffix_len;
            real_suffix_mask--;
        }
        return (suffix & real_suffix_mask);
    }

    SuffixType getType() const {
	return type_;
    }

    level_t getSuffixLen() const {
	return hash_suffix_len_ + real_suffix_len_;
    }

    level_t getHashSuffixLen() const {
	return hash_suffix_len_;
    }

    level_t getRealSuffixLen() const {
	return real_suffix_len_;
    }

    position_t serializedSize() const {
	position_t total_size = sizeof(num_bits_) + sizeof(type_)
            + sizeof(hash_suffix_len_) + sizeof(real_suffix_len_) + bitsSize();
	sizeAlign(total_size);
	return total_size;
    }

    position_t size() const {
	return (sizeof(BitvectorSuffix) + bitsSize());
    }

    word_t read(const position_t idx) const;
    word_t readReal(const position_t idx) const;
    bool checkEquality(const position_t idx, const std::string& key, const level_t level) const;

    // Compare stored suffix to querying suffix.
    // kReal suffix type only.
    int compare(const position_t idx, const std::string& key, const level_t level) const;

    void serialize(char*& dst) const {
	memcpy(dst, &num_bits_, sizeof(num_bits_));
	dst += sizeof(num_bits_);
	memcpy(dst, &type_, sizeof(type_));
	dst += sizeof(type_);
	memcpy(dst, &hash_suffix_len_, sizeof(hash_suffix_len_));
	dst += sizeof(hash_suffix_len_);
        memcpy(dst, &real_suffix_len_, sizeof(real_suffix_len_));
	dst += sizeof(real_suffix_len_);
	if (type_ != kNone) {
	    memcpy(dst, bits_, bitsSize());
	    dst += bitsSize();
	}
	align(dst);
    }

    static BitvectorSuffix* deSerialize(char*& src) {
	BitvectorSuffix* sv = new BitvectorSuffix();
	memcpy(&(sv->num_bits_), src, sizeof(sv->num_bits_));
	src += sizeof(sv->num_bits_);
	memcpy(&(sv->type_), src, sizeof(sv->type_));
	src += sizeof(sv->type_);
	memcpy(&(sv->hash_suffix_len_), src, sizeof(sv->hash_suffix_len_));
	src += sizeof(sv->hash_suffix_len_);
        memcpy(&(sv->real_suffix_len_), src, sizeof(sv->real_suffix_len_));
	src += sizeof(sv->real_suffix_len_);
	if (sv->type_ != kNone) {
	    sv->bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(src));
	    src += sv->bitsSize();
	}
	align(src);
	return sv;
    }

    void destroy() {
	if (type_ != kNone)
	    delete[] bits_;
    }

private:
    SuffixType type_;
    level_t hash_suffix_len_; // in bits
    level_t real_suffix_len_; // in bits
};

}
#endif // SUFFIXVECTOR_H_
