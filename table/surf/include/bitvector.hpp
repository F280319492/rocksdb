#ifndef BITVECTOR_H_
#define BITVECTOR_H_

#include <assert.h>

#include <vector>

#include "config.hpp"

namespace surf {

class Bitvector {
public:
    Bitvector() : num_bits_(0), bits_(nullptr) {};

    Bitvector(const std::vector<std::vector<word_t> >& bitvector_per_level, 
	      const std::vector<position_t>& num_bits_per_level, 
	      const level_t start_level = 0, 
	      level_t end_level = 0/* non-inclusive */) {
	if (end_level == 0)
	    end_level = bitvector_per_level.size();
	num_bits_ = totalNumBits(num_bits_per_level, start_level, end_level);
	bits_ = new word_t[numWords()];
	memset(bits_, 0, bitsSize());
	concatenateBitvectors(bitvector_per_level, num_bits_per_level, start_level, end_level);
    }

    ~Bitvector() {}

    position_t numBits() const {
	return num_bits_;
    }

    position_t numWords() const {
	if (num_bits_ % kWordSize == 0)
	    return (num_bits_ / kWordSize);
	else
	    return (num_bits_ / kWordSize + 1);
    }

    // in bytes
    position_t bitsSize() const {
	return (numWords() * (kWordSize / 8));
    }

    // in bytes
    position_t size() const {
	return (sizeof(Bitvector) + bitsSize());
    }

    bool readBit(const position_t pos) const;

    position_t distanceToNextSetBit(const position_t pos) const;
    position_t distanceToPrevSetBit(const position_t pos) const;

private:
    position_t totalNumBits(const std::vector<position_t>& num_bits_per_level, 
			    const level_t start_level, 
			    const level_t end_level/* non-inclusive */);

    void concatenateBitvectors(const std::vector<std::vector<word_t> >& bitvector_per_level, 
			       const std::vector<position_t>& num_bits_per_level, 
			       const level_t start_level, 
			       const level_t end_level/* non-inclusive */);
protected:
    position_t num_bits_;
    word_t* bits_;
};

}
#endif // BITVECTOR_H_
