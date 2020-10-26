#ifndef SURFBUILDER_H_
#define SURFBUILDER_H_

#include <assert.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "hash.hpp"
#include "suffix.hpp"

namespace surf {

class SuRFBuilder {
public: 
    SuRFBuilder() : sparse_start_level_(0), suffix_type_(kNone) {};
    explicit SuRFBuilder(bool include_dense, uint32_t sparse_dense_ratio,
			 SuffixType suffix_type, level_t hash_suffix_len, level_t real_suffix_len)
	: include_dense_(include_dense), sparse_dense_ratio_(sparse_dense_ratio),
	  sparse_start_level_(0), suffix_type_(suffix_type),
          hash_suffix_len_(hash_suffix_len), real_suffix_len_(real_suffix_len) {};

    ~SuRFBuilder() {};

    // Fills in the LOUDS-dense and sparse vectors (members of this class)
    // through a single scan of the sorted key list.
    // After build, the member vectors are used in SuRF constructor.
    // REQUIRED: provided key list must be sorted.
    void build(const std::vector<std::string>& keys);
    void build(const std::vector<std::string>& keys, 
                const std::vector<uint64_t>& suffixes,
                const std::vector<uint64_t>& vals);

    static bool readBit(const std::vector<word_t>& bits, const position_t pos) {
	assert(pos < (bits.size() * kWordSize));
	position_t word_id = pos / kWordSize;
	position_t offset = pos % kWordSize;
	return (bits[word_id] & (kMsbMask >> offset));
    }

    static void setBit(std::vector<word_t>& bits, const position_t pos) {
	assert(pos < (bits.size() * kWordSize));
	position_t word_id = pos / kWordSize;
	position_t offset = pos % kWordSize;
	bits[word_id] |= (kMsbMask >> offset);
    }

    level_t getTreeHeight() const {
	return labels_.size();
    }

    // const accessors
    const std::vector<std::vector<word_t> >& getBitmapLabels() const {
	return bitmap_labels_;
    }
    const std::vector<std::vector<word_t> >& getBitmapChildIndicatorBits() const {
	return bitmap_child_indicator_bits_;
    }
    const std::vector<std::vector<word_t> >& getPrefixkeyIndicatorBits() const {
	return prefixkey_indicator_bits_;
    }
    const std::vector<std::vector<label_t> >& getLabels() const {
	return labels_;
    }
    const std::vector<std::vector<word_t> >& getChildIndicatorBits() const {
	return child_indicator_bits_;
    }
    const std::vector<std::vector<word_t> >& getLoudsBits() const {
	return louds_bits_;
    }
    const std::vector<std::vector<word_t> >& getSuffixes() const {
	return suffixes_;
    }
    const std::vector<std::vector<word_t> >& getValues() const {
        return values_;
    }
    const std::vector<position_t>& getSuffixCounts() const {
	return suffix_counts_;
    }
    const std::vector<position_t>& getValueCounts() const {
        return suffix_counts_;
    }
    const std::vector<position_t>& getNodeCounts() const {
	return node_counts_;
    }
    level_t getSparseStartLevel() const {
	return sparse_start_level_;
    }
    SuffixType getSuffixType() const {
	return suffix_type_;
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

private:
    static bool isSameKey(const std::string& a, const std::string& b) {
	return a.compare(b) == 0;
    }

    // Fill in the LOUDS-Sparse vectors through a single scan
    // of the sorted key list.
    void buildSparse(const std::vector<std::string>& keys);
    void buildSparse(const std::vector<std::string>& keys,
                        const std::vector<uint64_t>& suffixes,
                        const std::vector<uint64_t>& vals);

    // Walks down the current partially-filled trie by comparing key to
    // its previous key in the list until their prefixes do not match.
    // The previous key is stored as the last items in the per-level 
    // label vector.
    // For each matching prefix byte(label), it sets the corresponding
    // child indicator bit to 1 for that label.
    level_t skipCommonPrefix(const std::string& key);

    // Starting at the start_level of the trie, the function inserts 
    // key bytes to the trie vectors until the first byte/label where 
    // key and next_key do not match.
    // This function is called after skipCommonPrefix. Therefore, it
    // guarantees that the stored prefix of key is unique in the trie.
    level_t insertKeyBytesToTrieUntilUnique(const std::string& key, const std::string& next_key, const level_t start_level);

    // Fills in the suffix byte for key
    inline void insertSuffix(const std::string& key, const level_t level);
    inline void insertValToSuffix(const uint64_t val, const level_t level);
    inline void insertValToValue(const uint64_t val, const level_t level);

    inline bool isCharCommonPrefix(const label_t c, const level_t level) const;
    inline bool isLevelEmpty(const level_t level) const;
    inline void moveToNextItemSlot(const level_t level);
    void insertKeyByte(const char c, const level_t level, const bool is_start_of_node, const bool is_term);
    inline void storeSuffix(const level_t level, const word_t suffix);
    inline void storeValue(const level_t level, const word_t suffix);

    // Compute sparse_start_level_ according to the pre-defined
    // size ratio between Sparse and Dense levels.
    // Dense size < Sparse size / sparse_dense_ratio_
    inline void determineCutoffLevel();

    inline uint64_t computeDenseMem(const level_t downto_level) const;
    inline uint64_t computeSparseMem(const level_t start_level) const;
    
    // Fill in the LOUDS-Dense vectors based on the built
    // Sparse vectors.
    // Called after sparse_start_level_ is set.
    void buildDense();

    void initDenseVectors(const level_t level);
    void setLabelAndChildIndicatorBitmap(const level_t level, const position_t node_num, const position_t pos);

    position_t getNumItems(const level_t level) const;
    void addLevel();
    bool isStartOfNode(const level_t level, const position_t pos) const;
    bool isTerminator(const level_t level, const position_t pos) const;

private:
    // trie level < sparse_start_level_: LOUDS-Dense
    // trie level >= sparse_start_level_: LOUDS-Sparse
    bool include_dense_;
    uint32_t sparse_dense_ratio_;
    level_t sparse_start_level_;

    // LOUDS-Sparse bit/byte vectors
    std::vector<std::vector<label_t> > labels_;
    std::vector<std::vector<word_t> > child_indicator_bits_;
    std::vector<std::vector<word_t> > louds_bits_;

    // LOUDS-Dense bit vectors
    std::vector<std::vector<word_t> > bitmap_labels_;
    std::vector<std::vector<word_t> > bitmap_child_indicator_bits_;
    std::vector<std::vector<word_t> > prefixkey_indicator_bits_;

    SuffixType suffix_type_;
    level_t hash_suffix_len_;
    level_t real_suffix_len_;
    std::vector<std::vector<word_t> > suffixes_;
    std::vector<position_t> suffix_counts_;

    level_t value_len_;
    std::vector<std::vector<word_t> > values_;
    std::vector<position_t> value_counts_;

    // auxiliary per level bookkeeping vectors
    std::vector<position_t> node_counts_;
    std::vector<bool> is_last_item_terminator_;
};


}
#endif // SURFBUILDER_H_
