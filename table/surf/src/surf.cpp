#include "../include/surf.hpp"

namespace surf {

void SuRF::create(const std::vector<std::string>& keys, 
		  const bool include_dense, const uint32_t sparse_dense_ratio,
		  const SuffixType suffix_type,
                  const level_t hash_suffix_len, const level_t real_suffix_len) {
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio,
                              suffix_type, hash_suffix_len, real_suffix_len);
    builder_->build(keys);
    louds_dense_ = new LoudsDense(builder_);
    louds_sparse_ = new LoudsSparse(builder_);
    iter_ = SuRF::Iter(this);
    delete builder_;
}

void SuRF::create(const std::vector<std::string>& keys,
                    const std::vector<uint64_t>& suffixs,
                    const std::vector<uint64_t>& vals,
                    const bool include_dense, 
                    const uint32_t sparse_dense_ratio,
                    const SuffixType suffix_type,
                    const level_t hash_suffix_len, 
                    const level_t real_suffix_len) {
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio,
                              suffix_type, hash_suffix_len, real_suffix_len);
    builder_->build(keys, suffixs, vals);
    louds_dense_ = new LoudsDense(builder_);
    louds_sparse_ = new LoudsSparse(builder_);
    iter_ = SuRF::Iter(this);
    delete builder_;
}

bool SuRF::lookupKey(const std::string& key) const {
    position_t connect_node_num = 0;
    if (!louds_dense_->lookupKey(key, connect_node_num))
	return false;
    else if (connect_node_num != 0)
	return louds_sparse_->lookupKey(key, connect_node_num);
    return true;
}

SuRF::Iter SuRF::moveToKeyGreaterThan(const std::string& key, const bool inclusive) const {
    SuRF::Iter iter(this);
    iter.could_be_fp_ = louds_dense_->moveToKeyGreaterThan(key, inclusive, iter.dense_iter_);

    if (!iter.dense_iter_.isValid())
	    return iter;
    if (iter.dense_iter_.isComplete())
	    return iter;

    if (!iter.dense_iter_.isSearchComplete()) {
	    iter.passToSparse();
	    iter.could_be_fp_ = louds_sparse_->moveToKeyGreaterThan(key, inclusive, iter.sparse_iter_);
	    if (!iter.sparse_iter_.isValid())
	        iter.incrementDenseIter();
	    return iter;
    } else if (!iter.dense_iter_.isMoveLeftComplete()) {
	    iter.passToSparse();
	    iter.sparse_iter_.moveToLeftMostKey();
	    return iter;
    }

    assert(false); // shouldn't reach here
    return iter;
}

SuRF::Iter SuRF::moveToKeyLessThan(const std::string& key, const bool inclusive) const {
    SuRF::Iter iter = moveToKeyGreaterThan(key, false);
    if (!iter.isValid()) {
	iter = moveToLast();
	return iter;
    }
    if (!iter.getFpFlag()) {
	iter--;
	if (lookupKey(key))
	    iter--;
    }
    return iter;
}

SuRF::Iter SuRF::moveToFirst() const {
    SuRF::Iter iter(this);
    if (louds_dense_->getHeight() > 0) {
iter.dense_iter_.setToFirstLabelInRoot();
	iter.dense_iter_.moveToLeftMostKey();
	if (iter.dense_iter_.isMoveLeftComplete())
	    return iter;
	iter.passToSparse();
	iter.sparse_iter_.moveToLeftMostKey();
    } else {
	iter.sparse_iter_.setToFirstLabelInRoot();
	iter.sparse_iter_.moveToLeftMostKey();
    }
    return iter;
}

SuRF::Iter SuRF::moveToLast() const {
    SuRF::Iter iter(this);
    if (louds_dense_->getHeight() > 0) {
	iter.dense_iter_.setToLastLabelInRoot();
	iter.dense_iter_.moveToRightMostKey();
	if (iter.dense_iter_.isMoveRightComplete())
	    return iter;
	iter.passToSparse();
	iter.sparse_iter_.moveToRightMostKey();
    } else {
	iter.sparse_iter_.setToLastLabelInRoot();
	iter.sparse_iter_.moveToRightMostKey();
    }
    return iter;
}

bool SuRF::lookupRange(const std::string& left_key, const bool left_inclusive, 
		       const std::string& right_key, const bool right_inclusive) {
    iter_.clear();
    louds_dense_->moveToKeyGreaterThan(left_key, left_inclusive, iter_.dense_iter_);
    if (!iter_.dense_iter_.isValid()) return false;
    if (!iter_.dense_iter_.isComplete()) {
	if (!iter_.dense_iter_.isSearchComplete()) {
	    iter_.passToSparse();
	    louds_sparse_->moveToKeyGreaterThan(left_key, left_inclusive, iter_.sparse_iter_);
	    if (!iter_.sparse_iter_.isValid()) {
		iter_.incrementDenseIter();
	    }
	} else if (!iter_.dense_iter_.isMoveLeftComplete()) {
	    iter_.passToSparse();
	    iter_.sparse_iter_.moveToLeftMostKey();
	}
    }
    if (!iter_.isValid()) return false;
    int compare = iter_.compare(right_key);
    if (compare == kCouldBePositive)
	return true;
    if (right_inclusive)
	return (compare <= 0);
    else
	return (compare < 0);
}

uint64_t SuRF::approxCount(const SuRF::Iter* iter, const SuRF::Iter* iter2) {
    if (!iter->isValid() || !iter2->isValid()) return 0;
    position_t out_node_num_left = 0, out_node_num_right = 0;
    uint64_t count = louds_dense_->approxCount(&(iter->dense_iter_),
					       &(iter2->dense_iter_),
					       out_node_num_left,
					       out_node_num_right);
    count += louds_sparse_->approxCount(&(iter->sparse_iter_),
					&(iter2->sparse_iter_),
					out_node_num_left,
					out_node_num_right);
    return count;
}

uint64_t SuRF::approxCount(const std::string& left_key,
			   const std::string& right_key) {
    iter_.clear(); iter2_.clear();
    iter_ = moveToKeyGreaterThan(left_key, true);
    if (!iter_.isValid()) return 0;
    iter2_ = moveToKeyGreaterThan(right_key, true);
    if (!iter2_.isValid())
	iter2_ = moveToLast();

    return approxCount(&iter_, &iter2_);
}

uint64_t SuRF::serializedSize() const {
    return (louds_dense_->serializedSize()
	    + louds_sparse_->serializedSize());
}

uint64_t SuRF::getMemoryUsage() const {
    return (sizeof(SuRF) + louds_dense_->getMemoryUsage() + louds_sparse_->getMemoryUsage());
}

level_t SuRF::getHeight() const {
    return louds_sparse_->getHeight();
}

level_t SuRF::getSparseStartLevel() const {
    return louds_sparse_->getStartLevel();
}

//============================================================================

void SuRF::Iter::clear() {
    dense_iter_.clear();
    sparse_iter_.clear();
}

bool SuRF::Iter::getFpFlag() const {
    return could_be_fp_;
}

bool SuRF::Iter::isValid() const {
    return dense_iter_.isValid() 
	&& (dense_iter_.isComplete() || sparse_iter_.isValid());
}

int SuRF::Iter::compare(const std::string& key) const {
    assert(isValid());
    int dense_compare = dense_iter_.compare(key);
    if (dense_iter_.isComplete() || dense_compare != 0) 
	return dense_compare;
    return sparse_iter_.compare(key);
}

std::string SuRF::Iter::getKey() const {
    if (!isValid())
	return std::string();
    if (dense_iter_.isComplete())
	return dense_iter_.getKey();
    return dense_iter_.getKey() + sparse_iter_.getKey();
}

int SuRF::Iter::getSuffix(word_t* suffix) const {
    if (!isValid())
	return 0;
    if (dense_iter_.isComplete())
	return dense_iter_.getSuffix(suffix);
    return sparse_iter_.getSuffix(suffix);
}

int SuRF::Iter::getValue(word_t* val) const {
    if (!isValid())
        return 0;
    if (dense_iter_.isComplete())
        return dense_iter_.getValue(val);
    return sparse_iter_.getValue(val);
}

std::string SuRF::Iter::getKeyWithSuffix(unsigned* bitlen) const {
    *bitlen = 0;
    if (!isValid())
	return std::string();
    if (dense_iter_.isComplete())
	return dense_iter_.getKeyWithSuffix(bitlen);
    return dense_iter_.getKeyWithSuffix(bitlen) + sparse_iter_.getKeyWithSuffix(bitlen);
}

void SuRF::Iter::passToSparse() {
    sparse_iter_.setStartNodeNum(dense_iter_.getSendOutNodeNum());
}

bool SuRF::Iter::incrementDenseIter() {
    if (!dense_iter_.isValid()) 
	return false;

    dense_iter_++;
    if (!dense_iter_.isValid()) 
	return false;
    if (dense_iter_.isMoveLeftComplete()) 
	return true;

    passToSparse();
    sparse_iter_.moveToLeftMostKey();
    return true;
}

bool SuRF::Iter::incrementSparseIter() {
    if (!sparse_iter_.isValid()) 
	return false;
    sparse_iter_++;
    return sparse_iter_.isValid();
}

bool SuRF::Iter::operator ++(int) {
    if (!isValid()) 
	return false;
    if (incrementSparseIter()) 
	return true;
    return incrementDenseIter();
}

bool SuRF::Iter::decrementDenseIter() {
    if (!dense_iter_.isValid()) 
	return false;

    dense_iter_--;
    if (!dense_iter_.isValid()) 
	return false;
    if (dense_iter_.isMoveRightComplete()) 
	return true;

    passToSparse();
    sparse_iter_.moveToRightMostKey();
    return true;
}

bool SuRF::Iter::decrementSparseIter() {
    if (!sparse_iter_.isValid()) 
	return false;
    sparse_iter_--;
    return sparse_iter_.isValid();
}

bool SuRF::Iter::operator --(int) {
    if (!isValid()) 
	return false;
    if (decrementSparseIter()) 
	return true;
    return decrementDenseIter();
}

} // namespace surf

