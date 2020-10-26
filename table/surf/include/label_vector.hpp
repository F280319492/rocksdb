#ifndef LABELVECTOR_H_
#define LABELVECTOR_H_

#include <emmintrin.h>

#include <vector>

#include "config.hpp"

namespace surf {

class LabelVector {
public:
    LabelVector() : num_bytes_(0), labels_(nullptr) {};

    LabelVector(const std::vector<std::vector<label_t> >& labels_per_level,
		const level_t start_level = 0,
		level_t end_level = 0/* non-inclusive */) {
	if (end_level == 0)
	    end_level = labels_per_level.size();

	num_bytes_ = 1;
	for (level_t level = start_level; level < end_level; level++)
	    num_bytes_ += labels_per_level[level].size();

	labels_ = new label_t[num_bytes_];

	position_t pos = 0;
	for (level_t level = start_level; level < end_level; level++) {
	    for (position_t idx = 0; idx < labels_per_level[level].size(); idx++) {
		labels_[pos] = labels_per_level[level][idx];
		pos++;
	    }
	}
    }

    ~LabelVector() {}

    position_t getNumBytes() const {
	return num_bytes_;
    }

    position_t serializedSize() const {
	position_t total_size = sizeof(num_bytes_) + num_bytes_;
	sizeAlign(total_size);
	return total_size;
    }

    position_t size() const {
	return (sizeof(LabelVector) + num_bytes_);
    }

    label_t read(const position_t pos) const {
	return labels_[pos];
    }

    label_t operator[](const position_t pos) const {
	return labels_[pos];
    }

    bool search(const label_t target, position_t& pos, const position_t search_len) const;
    bool searchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const;

    bool binarySearch(const label_t target, position_t& pos, const position_t search_len) const;
    bool simdSearch(const label_t target, position_t& pos, const position_t search_len) const;
    bool linearSearch(const label_t target, position_t& pos, const position_t search_len) const;

    bool binarySearchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const;
    bool linearSearchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const;

    void serialize(char*& dst) const {
	memcpy(dst, &num_bytes_, sizeof(num_bytes_));
	dst += sizeof(num_bytes_);
	memcpy(dst, labels_, num_bytes_);
	dst += num_bytes_;
	align(dst);
    }
    
    static LabelVector* deSerialize(char*& src) {
	LabelVector* lv = new LabelVector();
	memcpy(&(lv->num_bytes_), src, sizeof(lv->num_bytes_));
	src += sizeof(lv->num_bytes_);
	lv->labels_ = const_cast<label_t*>(reinterpret_cast<const label_t*>(src));
	src += lv->num_bytes_;
	align(src);
	return lv;
    }

    void destroy() {
	delete[] labels_;	
    }

private:
    position_t num_bytes_;
    label_t* labels_;
};

}
#endif // LABELVECTOR_H_
