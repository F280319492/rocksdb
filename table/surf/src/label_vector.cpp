#include "../include/label_vector.hpp"

namespace surf {

bool LabelVector::search(const label_t target, position_t& pos, position_t search_len) const {
    //skip terminator label
    if ((search_len > 1) && (labels_[pos] == kTerminator)) {
	pos++;
	search_len--;
    }

    if (search_len < 3)
	return linearSearch(target, pos, search_len);
    if (search_len < 12)
	return binarySearch(target, pos, search_len);
    else
	return simdSearch(target, pos, search_len);
}

bool LabelVector::searchGreaterThan(const label_t target, position_t& pos, position_t search_len) const {
    //skip terminator label
    if ((search_len > 1) && (labels_[pos] == kTerminator)) {
	pos++;
	search_len--;
    }

    if (search_len < 3)
	return linearSearchGreaterThan(target, pos, search_len);
    else
	return binarySearchGreaterThan(target, pos, search_len);
}

bool LabelVector::binarySearch(const label_t target, position_t& pos, const position_t search_len) const {
    position_t l = pos;
    position_t r = pos + search_len;
    while (l < r) {
	position_t m = (l + r) >> 1;
	if (target < labels_[m]) {
	    r = m;
	} else if (target == labels_[m]) {
	    pos = m;
	    return true;
	} else {
	    l = m + 1;
	}
    }
    return false;
}

bool LabelVector::simdSearch(const label_t target, position_t& pos, const position_t search_len) const {
    position_t num_labels_searched = 0;
    position_t num_labels_left = search_len;
    while ((num_labels_left >> 4) > 0) {
	label_t* start_ptr = labels_ + pos + num_labels_searched;
	__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(target), 
				     _mm_loadu_si128(reinterpret_cast<__m128i*>(start_ptr)));
	unsigned check_bits = _mm_movemask_epi8(cmp);
	if (check_bits) {
	    pos += (num_labels_searched + __builtin_ctz(check_bits));
	    return true;
	}
	num_labels_searched += 16;
	num_labels_left -= 16;
    }

    if (num_labels_left > 0) {
	label_t* start_ptr = labels_ + pos + num_labels_searched;
	__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(target), 
				     _mm_loadu_si128(reinterpret_cast<__m128i*>(start_ptr)));
	unsigned leftover_bits_mask = (1 << num_labels_left) - 1;
	unsigned check_bits = _mm_movemask_epi8(cmp) & leftover_bits_mask;
	if (check_bits) {
	    pos += (num_labels_searched + __builtin_ctz(check_bits));
	    return true;
	}
    }

    return false;
}

bool LabelVector::linearSearch(const label_t target, position_t&  pos, const position_t search_len) const {
    for (position_t i = 0; i < search_len; i++) {
	if (target == labels_[pos + i]) {
	    pos += i;
	    return true;
	}
    }
    return false;
}

bool LabelVector::binarySearchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const {
    position_t l = pos;
    position_t r = pos + search_len;
    while (l < r) {
	position_t m = (l + r) >> 1;
	if (target < labels_[m]) {
	    r = m;
	} else if (target == labels_[m]) {
	    if (m < pos + search_len - 1) {
		pos = m + 1;
		return true;
	    }
	    return false;
	} else {
	    l = m + 1;
	}
    }

    if (l < pos + search_len) {
	pos = l;
	return true;
    }
    return false;
}

bool LabelVector::linearSearchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const {
    for (position_t i = 0; i < search_len; i++) {
	if (labels_[pos + i] > target) {
	    pos += i;
	    return true;
	}
    }
    return false;
}

} // namespace surf

