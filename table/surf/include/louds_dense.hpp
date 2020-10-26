#ifndef LOUDSDENSE_H_
#define LOUDSDENSE_H_

#include <string>

#include "config.hpp"
#include "rank.hpp"
#include "suffix.hpp"
#include "surf_builder.hpp"

namespace surf {

class LoudsDense {
public:
    class Iter {
    public:
	Iter() : is_valid_(false) {};
	Iter(LoudsDense* trie) : is_valid_(false), is_search_complete_(false),
				 is_move_left_complete_(false),
				 is_move_right_complete_(false),
				 trie_(trie),
				 send_out_node_num_(0), key_len_(0),
				 is_at_prefix_key_(false) {
	    for (level_t level = 0; level < trie_->getHeight(); level++) {
		key_.push_back(0);
		pos_in_trie_.push_back(0);
	    }
	}

	void clear();
	bool isValid() const { return is_valid_; };
	bool isSearchComplete() const { return is_search_complete_; };
	bool isMoveLeftComplete() const { return is_move_left_complete_; };
	bool isMoveRightComplete() const { return is_move_right_complete_; };
	bool isComplete() const {
	    return (is_search_complete_ &&
		    (is_move_left_complete_ && is_move_right_complete_));
	}

	int compare(const std::string& key) const;
	std::string getKey() const;
	int getSuffix(word_t* suffix) const;
	int getValue(word_t* val) const;
    std::string getKeyWithSuffix(unsigned* bitlen) const;
	position_t getSendOutNodeNum() const { return send_out_node_num_; };

	void setToFirstLabelInRoot();
	void setToLastLabelInRoot();
	void moveToLeftMostKey();
	void moveToRightMostKey();
	void operator ++(int);
	void operator --(int);

    private:
	inline void append(position_t pos);
	inline void set(level_t level, position_t pos);
	inline void setSendOutNodeNum(position_t node_num) { send_out_node_num_ = node_num; };
	inline void setFlags(const bool is_valid, const bool is_search_complete, 
			     const bool is_move_left_complete,
			     const bool is_move_right_complete);

    private:
	// True means the iter either points to a valid key 
	// or to a prefix with length trie_->getHeight()
	bool is_valid_;
	// If false, call moveToKeyGreaterThan in LoudsSparse to complete
	bool is_search_complete_; 
	// If false, call moveToLeftMostKey in LoudsSparse to complete
	bool is_move_left_complete_;
	// If false, call moveToRightMostKey in LoudsSparse to complete
	bool is_move_right_complete_; 
	LoudsDense* trie_;
	position_t send_out_node_num_;
	level_t key_len_; // Does NOT include suffix

	std::vector<label_t> key_;
	std::vector<position_t> pos_in_trie_;
	bool is_at_prefix_key_;

	friend class LoudsDense;
    };

public:
    LoudsDense() {};
    LoudsDense(const SuRFBuilder* builder);

    ~LoudsDense() {}

    // Returns whether key exists in the trie so far
    // out_node_num == 0 means search terminates in louds-dense.
    bool lookupKey(const std::string& key, position_t& out_node_num) const;
    // return value indicates potential false positive
    bool moveToKeyGreaterThan(const std::string& key, 
			      const bool inclusive, LoudsDense::Iter& iter) const;
    uint64_t approxCount(const LoudsDense::Iter* iter_left,
			 const LoudsDense::Iter* iter_right,
			 position_t& out_node_num_left,
			 position_t& out_node_num_right) const;

    uint64_t getHeight() const { return height_; };
    uint64_t serializedSize() const;
    uint64_t getMemoryUsage() const;

    void serialize(char*& dst) const {
	    memcpy(dst, &height_, sizeof(height_));
	    dst += sizeof(height_);
	    memcpy(dst, level_cuts_, sizeof(position_t) * height_);
	    dst += (sizeof(position_t) * height_);
	    align(dst);
	    label_bitmaps_->serialize(dst);
	    child_indicator_bitmaps_->serialize(dst);
	    prefixkey_indicator_bits_->serialize(dst);
	    suffixes_->serialize(dst);
        values_->serialize(dst);
	    align(dst);
    }

    static LoudsDense* deSerialize(char*& src) {
	    LoudsDense* louds_dense = new LoudsDense();
	    memcpy(&(louds_dense->height_), src, sizeof(louds_dense->height_));
	    src += sizeof(louds_dense->height_);
    	louds_dense->level_cuts_ = new position_t[louds_dense->height_];
        memcpy(louds_dense->level_cuts_, src,
	        sizeof(position_t) * (louds_dense->height_));
	    src += (sizeof(position_t) * (louds_dense->height_));
	    align(src);
	    louds_dense->label_bitmaps_ = BitvectorRank::deSerialize(src);
	    louds_dense->child_indicator_bitmaps_ = BitvectorRank::deSerialize(src);
	    louds_dense->prefixkey_indicator_bits_ = BitvectorRank::deSerialize(src);
	    louds_dense->suffixes_ = BitvectorSuffix::deSerialize(src);
	    louds_dense->values_ = BitvectorSuffix::deSerialize(src);
        align(src);
	    return louds_dense;
    }

    void destroy() {
	    label_bitmaps_->destroy();
	    child_indicator_bitmaps_->destroy();
	    prefixkey_indicator_bits_->destroy();
	    suffixes_->destroy();
        values_->destroy();
    }

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getSuffixPos(const position_t pos, const bool is_prefix_key) const;
    position_t getNextPos(const position_t pos) const;
    position_t getPrevPos(const position_t pos, bool* is_out_of_bound) const;

    bool compareSuffixGreaterThan(const position_t pos, const std::string& key, 
				  const level_t level, const bool inclusive, 
				  LoudsDense::Iter& iter) const;
    void extendPosList(std::vector<position_t>& pos_list,
		       position_t& out_node_num) const;

private:
    static const position_t kNodeFanout = 256;
    static const position_t kRankBasicBlockSize  = 512;

    level_t height_;
    position_t* level_cuts_; // position of the last bit at each level

    BitvectorRank* label_bitmaps_;
    BitvectorRank* child_indicator_bitmaps_;
    BitvectorRank* prefixkey_indicator_bits_; //1 bit per internal node
    BitvectorSuffix* suffixes_;
    BitvectorSuffix* values_;
};

}
#endif // LOUDSDENSE_H_
