/*
 * Copyright (C) 2005-2014 Christoph Rupp (chris@crupp.de).
 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property
 * of Christoph Rupp and his suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Christoph Rupp
 * and his suppliers and may be covered by Patents, patents in process,
 * and are protected by trade secret or copyright law. Dissemination of
 * this information or reproduction of this material is strictly forbidden
 * unless prior written permission is obtained from Christoph Rupp.
 */

/*
 * Compressed 32bit integer keys
 *
 * @exception_safe: strong
 * @thread_safe: no
 */

#ifndef HAM_BTREE_KEYS_ZINT32_H
#define HAM_BTREE_KEYS_ZINT32_H

#include <sstream>
#include <iostream>

#include "0root/root.h"

// Always verify that a file of level N does not include headers > N!
#include "3btree/btree_node.h"
#include "3btree/btree_keys_base.h"

#ifndef HAM_ROOT_H
#  error "root.h was not included"
#endif

namespace hamsterdb {

//
// The template classes in this file are wrapped in a separate namespace
// to avoid naming clashes with btree_impl_default.h
//
namespace Zint32 {

/*
 * A helper class to sort ranges; used in vacuumize()
 */
struct SortHelper {
  uint32_t offset;
  int index;

  bool operator<(const SortHelper &rhs) const {
    return (offset < rhs.offset);
  }
};

static bool
sort_by_offset(const SortHelper &lhs, const SortHelper &rhs) {
  return (lhs.offset < rhs.offset);
}

class Zint32KeyList : public BaseKeyList
{
    // This structure is an "index" entry which describes the location
    // of a variable-length block
    struct Index {
      // offset of the payload, relative to the beginning of the payloads
      // (past the Index structures)
      // TODO needs to have more bytes for larger pages!
      uint16_t offset;

      // used size of this block
      uint16_t used_size;

      // the size of this block, in multiples of kBlockSizeFactor
      uint8_t block_size_mul;

      // the number of keys in this block
      uint8_t key_count;

      // helper function which returns the absolute payload size
      int block_size() const {
        return (block_size_mul * kBlockSizeFactor);
      }
    };

  public:
    typedef uint32_t type;

    enum {
      // A flag whether this KeyList has sequential data
      kHasSequentialData = 0,

      // A flag whether this KeyList supports the scan() call
      kSupportsBlockScans = 1,

      kBlockSizeFactor = 32,
      kMaxBlockSize = 64,
      kInitialBlockSize = 1,
    };

    // Constructor
    Zint32KeyList(LocalDatabase *db)
      : m_data(0), m_range_size(0) {
    }

    // Creates a new KeyList starting at |ptr|, total size is
    // |full_range_size_bytes| (in bytes)
    void create(uint8_t *data, size_t range_size) {
      m_data = data;
      m_range_size = range_size;

      // a new node starts with an empty block
      set_block_count(0);
      set_used_size(sizeof(uint32_t) * 2);
      add_block(0);
    }

    // Opens an existing KeyList
    void open(uint8_t *data, size_t range_size, size_t node_count) {
      m_data = data;
      m_range_size = range_size;
    }

    // Calculates the required size for a range
    size_t get_required_range_size(size_t node_count) const {
      return (get_used_size());
    }

    // Returns the actual key size including overhead. This is an estimate.
    size_t get_full_key_size(const ham_key_t *key = 0) const {
      return (3);
    }

    // Returns true if the |key| no longer fits into the node
    bool requires_split(size_t node_count, const ham_key_t *key) const {
      // worst worst case: a block is split
      // TODO this doesn't make sense. Merge requires_split() with insert()
      // then we won't run into this situation
      return (get_used_size() + kBlockSizeFactor * 4 >= m_range_size);
    }

    // Rearranges the list
    void vacuumize(size_t node_count, bool force) {
      ham_assert(check_integrity(node_count));
      ham_assert(get_block_count() > 0);

      // make a copy of all indices
      bool requires_sort = false;
      int block_count = get_block_count();
	  SortHelper *s = (SortHelper *)::alloca(block_count * sizeof(SortHelper));
      for (int i = 0; i < block_count; i++) {
        s[i].index = i;
        s[i].offset = get_block_index(i)->offset;
        if (i > 0 && requires_sort == false && s[i].offset < s[i - 1].offset)
          requires_sort = true;
      }

      // sort them by offset; this is a very expensive call. only sort if
      // it's absolutely necessary!
      if (requires_sort)
        std::sort(&s[0], &s[block_count], sort_by_offset);

      // shift all blocks "to the left" and reduce their size as much as
      // possible
      uint32_t next_offset = 0;
      uint8_t *block_data = &m_data[8 + sizeof(Index) * block_count];

      for (int i = 0; i < block_count; i++) {
        Index *index = get_block_index(s[i].index);

        int block_size_req = index->used_size / kBlockSizeFactor;
        if (block_size_req == 0
            || block_size_req * kBlockSizeFactor < index->used_size)
          block_size_req++;
        ham_assert(block_size_req <= index->block_size_mul);

        if (index->offset != next_offset) {
          // shift block data to the left
          memmove(&block_data[next_offset], &block_data[index->offset],
                          index->used_size);
          // overwrite the offset
          index->offset = next_offset;
        }

        index->block_size_mul = block_size_req;
        next_offset += index->block_size();
      }

      set_used_size((block_data - m_data) + next_offset);
      ham_assert(check_integrity(node_count));
    }

    // Change the range size
    void change_range_size(size_t node_count, uint8_t *new_data_ptr,
                    size_t new_range_size, size_t capacity_hint) {
      if (m_data != new_data_ptr) {
        ::memmove(new_data_ptr, m_data, get_used_size());

        m_data = new_data_ptr;
      }
      m_range_size = new_range_size;
    }

    // Copies a key into |dest|
    void get_key(int slot, ByteArray *arena, ham_key_t *dest,
                    bool deep_copy = true) {
      // uncompress the key value, store it in a member (not in a local
      // variable!)
      m_dummy = value(slot);

      dest->size = sizeof(uint32_t);
      if (deep_copy == false) {
        dest->data = (uint8_t *)&m_dummy;
        return;
      }

      // allocate memory (if required)
      if (!(dest->flags & HAM_KEY_USER_ALLOC)) {
        arena->resize(dest->size);
        dest->data = arena->get_ptr();
      }

      ::memcpy(dest->data, &m_dummy, sizeof(uint32_t));
    }

    // Returns the threshold when switching from binary search to
    // linear search; currently not supported
    size_t get_linear_search_threshold() const {
      return ((size_t)-1);
    }

    // Performs a linear search in a given range between |start| and
    // |start + length|
    template<typename Cmp>
    int linear_search(size_t start, size_t length, ham_key_t *hkey,
                    Cmp &comparator, int *pcmp) {
      ham_assert(!"shouldn't be here");
      throw Exception(HAM_NOT_IMPLEMENTED);
    }

    // Inserts a key
    void insert(size_t node_count, int slot, const ham_key_t *key) {
      ham_assert(check_integrity(node_count));
      ham_assert(key->size == sizeof(uint32_t));

      // find the block
      int position_in_block;
      Index *index;
      if (slot == 0) {
        index = get_block_index(0);
        position_in_block = 0;
      }
      else if (slot == (int)node_count) {
        index = get_block_index(get_block_count() - 1);
        position_in_block = index->key_count;
      }
      else
        index = find_block_by_slot(slot, &position_in_block);

      // if the block is full then grow or split it
      if (index->used_size + 5 > index->block_size()) {
        // already reached max. size? then perform a split
        if (index->block_size() > kMaxBlockSize) {
          if (slot == 0) {
            index = add_block(0);
          }
          else if (position_in_block == index->key_count) {
            index = add_block(get_block_count());
            position_in_block = 0;
          }
          else {
            index = split_block(index, *(uint32_t *)key->data,
                            &position_in_block);
          }
        }
        // otherwise grow the block
        else {
          grow_block(index);
        }
      }

      // first key in an empty block? then don't store a delta
      if (index->key_count == 0) {
        index->key_count = 1;
        uint8_t *p = get_block_data(index);
        *(uint32_t *)p = *(uint32_t *)key->data;
        index->used_size = sizeof(uint32_t);
        ham_assert(check_integrity(node_count + 1));
        return;
      }

      // now prepend, append or insert
      if (position_in_block == 0)
        prepend_key_to_block(index, *(uint32_t *)key->data);
      else if (position_in_block == index->key_count)
        append_key_to_block(index, *(uint32_t *)key->data);
      else
        insert_key_in_block(index, position_in_block, *(uint32_t *)key->data);

      ham_assert(check_integrity(node_count + 1));
    }

    // Erases a key
    void erase(size_t node_count, int slot) {
      ham_assert(check_integrity(node_count));

      int position_in_block;
      Index *index;
      if (slot == 0) {
        index = get_block_index(0);
        position_in_block = 0;
      }
      else if (slot == (int)node_count) {
        index = get_block_index(get_block_count() - 1);
        position_in_block = index->key_count;
      }
      else
        index = find_block_by_slot(slot, &position_in_block);

      // is there just one key left in that block? then reduce the counters
      if (index->key_count == 1) {
        index->key_count = 0;
        index->used_size = 0;
      }
      // otherwise reduce the block
      else {
        erase_key_from_block(index, position_in_block);
      }

      // if the block is now empty then remove it, unless it's the last block
      if (index->key_count == 0 && get_block_count() > 0) {
        remove_block(index);
      }

      ham_assert(check_integrity(node_count - 1));
    }

    // Copies all keys from this[sstart] to dest[dstart]; this is
    // for splitting nodes
    void copy_to(int sstart, size_t node_count, Zint32KeyList &dest,
                    size_t other_count, int dstart) {
      ham_assert(check_integrity(node_count));

      Index *index = 0;
      
      // find the start block 
      int src_position_in_block;
      Index *srci = find_block_by_slot(sstart, &src_position_in_block);
      Index *dsti = 0;

      if (src_position_in_block > 0) {
        src_position_in_block++;

        // if the start position is in the middle of a block then fast-forward
        // to that position
        uint8_t *s;
        uint32_t srckey = 0;
        s = fast_forward(srci, src_position_in_block, &srckey);

        // need to keep a copy of the pointer where we started, so we can later
        // figure out how many bytes were copied
        uint8_t *start_s = s;

        // get the destination block
        int dst_position_in_block;
        dsti = dest.find_block_by_slot(dstart, &dst_position_in_block);

        // make sure it has free space TODO
        dest.grow_block(dsti);
        dest.grow_block(dsti);

        // fast-forward to the start position in the destination block
        uint8_t *d;
        uint32_t dstkey = 0;
        if (dst_position_in_block > 0) {
          d = fast_forward(dsti, dst_position_in_block, &dstkey);
        }
        // otherwise get a pointer to the beginning of the data
        else {
          d = dest.get_block_data(dsti);
        }

        // need to keep a copy of the pointer where we started, so we can later
        // figure out how many bytes were copied
        uint8_t *start_d = d;

        // now copy the first key. take into account that the first key of
        // a block is uncompressed.
        if (src_position_in_block == 0) {
          srckey = *(uint32_t *)s;
          s += sizeof(uint32_t);
          src_position_in_block = 1;
        }

        if (dst_position_in_block == 0) {
          *(uint32_t *)d = srckey;
          dstkey = srckey;
          d += sizeof(uint32_t);
        }
        else {
          d += write_int(d, srckey - dstkey);
        }
        dsti->key_count++;

        // now copy the remaining keys of the first block
        for (int i = src_position_in_block; i < srci->key_count; i++) {
          uint32_t delta;
          s += read_int(s, &delta);

          d += write_int(d, delta);
          dsti->key_count++;
        }
        srci->key_count -= dsti->key_count;
        srci->used_size -= s - start_s;
        dsti->used_size += d - start_d;

        index = srci + 1;
      }
      // |src_position_in_block| is > 0 - just fall through and the whole
      // block will be copied
      else {
        index = srci;
      }

      // now copy the remaining blocks
      int copied_blocks = 0;
      Index *endi = get_block_index(get_block_count()); 
      for (; index < endi; index++, copied_blocks++) {
        // TODO it would be faster to introduce add_many_blocks()
        // instead of invoking add_block() several times
        dsti = dest.add_block(dest.get_block_count(), index->block_size_mul);
        copy_blocks(index, dest, dsti);
      }

      // remove the copied blocks
      // TODO use remove_blocks()
      uint8_t *pend = &m_data[get_used_size()];
      uint8_t *pold = (uint8_t *)get_block_index(get_block_count());
      uint8_t *pnew = (uint8_t *)get_block_index(get_block_count()
                                    - copied_blocks);
      ::memmove(pnew,  pold, pend - pold);

      set_block_count(get_block_count() - copied_blocks);

      // now update get_used_size()
      index = get_block_index(0);
      Index *end = get_block_index(get_block_count());

      int used_size = 0;
      for (; index < end; index++) {
        if (index->offset + index->block_size() > used_size)
          used_size = index->offset + index->block_size();
      }

      // add static overhead
      set_used_size(used_size + 8 + sizeof(Index) * get_block_count());

      ham_assert(dest.check_integrity(node_count - sstart));
      ham_assert(check_integrity(sstart));
    }

    // Iterates all keys, calls the |visitor| on each
    void scan(ScanVisitor *visitor, uint32_t start, size_t count) {
      // TODO
      // int c = get_block_count();
      // for (int i = 0; i < c; i++) {
      //     uint32_t data[kMaxBlockLength];
      //     decompress_block(i, &data[0]);
      //     (*visitor)(data, get_key_count(i));
      // }
    }

    // Checks the integrity of this node. Throws an exception if there is a
    // violation.
    bool check_integrity(size_t node_count) const {
      ham_assert(get_block_count() > 0);
      Index *index = get_block_index(0);
      Index *end = get_block_index(get_block_count());

      size_t total_keys = 0;
      int used_size = 0;
      for (; index < end; index++) {
        total_keys += index->key_count;
        if (used_size < index->offset + index->block_size())
          used_size = index->offset + index->block_size();

        ham_assert(used_size % kBlockSizeFactor == 0);

        if (index->used_size > index->block_size()) {
          ham_trace(("Used block size %d exceeds allocated size %d",
                        (int)index->used_size,
                        (int)index->block_size()));
          throw Exception(HAM_INTEGRITY_VIOLATED);
        }
      }

      // add static overhead
      used_size += 8 + sizeof(Index) * get_block_count();

      if (used_size != (int)get_used_size()) {
        ham_log(("used size %d differs from expected %d",
                (int)used_size, (int)get_used_size()));
        throw Exception(HAM_INTEGRITY_VIOLATED);
      }

      if (used_size > (int)m_range_size) {
        ham_log(("used size %d exceeds range size %d",
                (int)used_size, (int)m_range_size));
        throw Exception(HAM_INTEGRITY_VIOLATED);
      }

      if (total_keys != node_count) {
        ham_log(("key count %d differs from expected %d",
                (int)total_keys, (int)node_count));
        throw Exception(HAM_INTEGRITY_VIOLATED);
      }

      return (true);
    }

    // Prints a key to |out| (for debugging)
    void print(int slot, std::stringstream &out) const {
      out << value(slot);
    }

    // Has support for SIMD style search?
    bool has_simd_support() const {
      return (false);
    }

    // Returns the pointer to the key's inline data - for SIMD calculations
    uint8_t *get_simd_data() {
      print_block(0);
      return (0); // TODO
    }

  private:
    // Prints all keys of a block to stdout (for debugging)
    void print_block(Index *index) const {
      uint8_t *p = get_block_data(index);
      uint32_t key = *(uint32_t *)p;
      std::cout << "0: " << key << std::endl;
      p += sizeof(uint32_t);

      uint32_t delta;
      for (int i = 1; i < index->key_count; i++) {
        p += read_int(p, &delta);
        key += delta;
        std::cout << i << ": " << key << std::endl;
      }
    }

    // Sets the block count
    void set_block_count(int count) {
      *(uint32_t *)m_data = (uint32_t)count;
    }

    // Returns the block count
    int get_block_count() const {
      return ((int) *(uint32_t *)m_data);
    }

    // Sets the used size of the range
    void set_used_size(size_t used_size) {
      *(uint32_t *)(m_data + 4) = (uint32_t)used_size;
    }

    // Returns the block count
    size_t get_used_size() const {
      return (*(uint32_t *)(m_data + 4));
    }

    // Returns a pointer to a block index
    Index *get_block_index(int i) const {
      return ((Index *)(m_data + 8 + i * sizeof(Index)));
    }

    // Returns the index for a block with that slot
    Index *find_block_by_slot(int slot, int *position_in_block) const {
      ham_assert(get_block_count() > 0);
      Index *index = get_block_index(0);
      Index *end = get_block_index(get_block_count());

      for (; index < end; index++) {
        if (index->key_count > slot) {
          *position_in_block = slot;
          return (index);
        } 

        slot -= index->key_count;
      }

      *position_in_block = slot;
      return (index - 1);
    }

    // Inserts a new block at the specified position
    Index *add_block(int position, int initial_size_mul = kInitialBlockSize) {
      // shift the whole data to the right to make space for the new block
      // index
      Index *index = get_block_index(position);

      if (get_block_count() != 0) {
        Index *dst = get_block_index(position + 1);
        ::memmove(dst, index, get_used_size() - (position * sizeof(Index)));
      }

      int initial_size_bytes = initial_size_mul * kBlockSizeFactor;
      set_block_count(get_block_count() + 1);
      set_used_size(get_used_size() + sizeof(Index) + initial_size_bytes);

      // initialize the new block index; the offset is relative to the start
      // of the payload data, and does not include the indices
      index->offset = get_used_size()
                - 2 * sizeof(uint32_t)
                - sizeof(Index) * get_block_count()
                - initial_size_bytes;
      index->block_size_mul = initial_size_mul;
      index->used_size = 0;
      index->key_count = 0;
      return (index);
    }

    // Copies two blocks; assumes that the new block |dst| has been properly
    // allocated
    void copy_blocks(Index *src, Zint32KeyList &dest, Index *dst) {
      dst->used_size = src->used_size;
      dst->key_count = src->key_count;

      ::memcpy(dest.get_block_data(dst), get_block_data(src), src->used_size);
    }

    // Removes the specified block
    void remove_block(Index *index) {
      ham_assert(get_block_count() > 1);
      ham_assert(index->key_count == 0);

      // shift all indices (and the payload data) to the left
      ::memmove(index + 1, index, get_used_size()
                    - sizeof(Index) * (index - get_block_index(0) + 1));
      set_block_count(get_block_count() - 1);
      set_used_size(get_used_size() - sizeof(Index));
    }

    // Splits a block; returns the index where the new key will be inserted,
    // |position_in_block| is adjusted
    Index *split_block(Index *index, uint32_t key, int *position_in_block) {
      int i;
      int block = index - get_block_index(0);
      // the new block will have the same size as the old one
      Index *new_index = add_block(block + 1, index->block_size_mul);

      uint8_t *src = get_block_data(index);
      uint32_t prev = *(uint32_t *)src;
      uint32_t delta;
      src += sizeof(uint32_t);

      // roughly skip half of the data
      // TODO don't loop but use msb to figure out where to split
      int size = index->used_size / 2 - sizeof(uint32_t);
      for (i = 1; i < index->key_count && size > 0; i++) {
        int delta_size = read_int(src, &delta);
        prev += delta;
        src += delta_size;
        size -= delta_size;
      }
      ham_assert(i < index->key_count);

      // the next delta will be the initial key of the new block
      uint8_t *dst = get_block_data(new_index);
      src += read_int(src, &delta);
      prev += delta;
      *(uint32_t *)dst = prev;
      dst += sizeof(uint32_t);

      // now copy the remaining data
      size = (get_block_data(index) + index->used_size) - src;
      memcpy(dst, src, size);
      dst += size;

      // and update all counters
      new_index->key_count = index->key_count - i;
      index->key_count = i;
      new_index->used_size = dst - get_block_data(new_index);
      index->used_size = src - get_block_data(index);

      // now figure out whether the key will be inserted in the old or
      // the new block
      if (key >= *(uint32_t *)get_block_data(new_index)) {
        *position_in_block -= i;
        return (new_index);
      }
      else {
        // position_in_block is still correct
        return (index);
      }
    }

    // Grows a block
    void grow_block(Index *index) {
      // move all other blocks unless the current block is the last one
      if ((size_t)index->offset + index->block_size()
              < get_used_size() - 8 - sizeof(Index) * get_block_count()) {
        uint8_t *p = get_block_data(index) + index->block_size();
        uint8_t *q = &m_data[get_used_size()];
        ::memmove(p + kBlockSizeFactor, p, q - p);

        // now update the offsets of the other blocks
        Index *next = get_block_index(0);
        Index *end = get_block_index(get_block_count());
        for (; next < end; next++)
          if (next->offset > index->offset)
            next->offset += kBlockSizeFactor;
      }

      index->block_size_mul += 1;
      set_used_size(get_used_size() + kBlockSizeFactor);
    }

    // Prepends a new key to a block
    void prepend_key_to_block(Index *index, uint32_t key) {
      ham_assert(index->key_count > 0);
      uint8_t *p = get_block_data(index);

      // the first key will be replaced with its delta
      uint32_t delta = *(uint32_t *)p - key;
      *(uint32_t *)p = key;
      p += sizeof(uint32_t);

      int required_space = calculate_delta_size(delta);

      // create a gap for the new key and the delta
      ::memmove(p + required_space, p, index->used_size - sizeof(uint32_t));

      // now write the delta
      write_int(p, delta);

      index->key_count++;
      index->used_size += required_space;
    }

    // Appends a new key to a block
    void append_key_to_block(Index *index, uint32_t key) {
      ham_assert(index->key_count > 0);

      // fast-forward to the end of the keys
      uint32_t prev;
      uint8_t *p = fast_forward(index, index->key_count, &prev);
              
      // now append the new delta value
      key -= prev;
      int size = write_int(p, key);

      index->key_count++;
      index->used_size += size;
    }

    // Inserts a new key in a block
    void insert_key_in_block(Index *index, int position, uint32_t key) {
      ham_assert(position < index->key_count);
      ham_assert(index->key_count > 0);

      // fast-forward to the end of the keys
      uint32_t prev;
      uint8_t *p = fast_forward(index, position, &prev);

      // we also need to read the next key at |position + 1|, because
      // its delta will change when the new key is inserted
      uint32_t next_key;
      uint8_t *next_p = 0;
      if (position < index->key_count) {
        next_p = p + read_int(p, &next_key);
        next_key += prev;
      }

      // how much additional space is required to store the delta of the
      // new key *and* the updated delta of the next key?
      int required_space = calculate_delta_size(key - prev);
      if (next_p) {
        required_space += calculate_delta_size(next_key - key);
        // minus the space that next_key currently occupies
        required_space -= next_p - p;
      }

      // create a gap large enough for the two deltas
      ::memmove(p + required_space, p, index->used_size
                      - (p - get_block_data(index)));

      // now insert the new key
      p += write_int(p, key - prev);
      // and the delta of the next key
      if (next_p)
        p += write_int(p, next_key - key);

      index->key_count++;
      index->used_size += required_space;
    }

    // Erases a key from a block
    void erase_key_from_block(Index *index, int position) {
      ham_assert(position < index->key_count);
      ham_assert(index->key_count > 1);

      uint8_t *p = get_block_data(index);

      // erase the first key?
      if (position == 0) {
        uint32_t second, first = *(uint32_t *)p;
        p += sizeof(uint32_t);
        uint8_t *q = p + read_int(p, &second);
        // replace the first key with the second key (uncompressed)
        *(uint32_t *)get_block_data(index) = first + second;
        // shift all remaining deltas to the left
        ::memmove(p, q, index->used_size - (q - p));
        index->used_size -= q - p;
        index->key_count--;
        return;
      }

      // otherwise fast-forward to the position of the key and remove it;
      // then update the delta of the next key
      uint32_t key = *(uint32_t *)p;
      p += sizeof(uint32_t);
      uint8_t *q = p;
      for (int i = 1; i < position; i++) {
        uint32_t delta;
        q = p;
        p += read_int(p, &delta);
        key += delta;
      }

      // if this was the last key then return
      if (position == index->key_count - 1) {
        index->used_size -= p - q;
        index->key_count--;
        return;
      }

      // read the next delta, it has to be updated
      uint32_t next_key;
      int size = read_int(p, &next_key);
      p += size;
      next_key += key;

      // |p| now points *behind* |next_key|, |q| points to the beginning
      // of the deleted key. |size| is the size of the old delta-value.
      q += write_int(q, next_key);
      ::memmove(q, p, index->used_size - (p - get_block_data(index)) - size);

      index->used_size -= size;
      index->key_count--;
    }

    // this assumes that there is a value to be read
    int read_int(const uint8_t *in, uint32_t *out) const {
      *out = in[0] & 0x7F;
      if (in[0] >= 128) {
        return (1);
      }
      *out = ((in[1] & 0x7FU) << 7) | *out;
      if (in[1] >= 128) {
        return (2);
      }
      *out = ((in[2] & 0x7FU) << 14) | *out;
      if (in[2] >= 128) {
        return (3);
      }
      *out = ((in[3] & 0x7FU) << 21) | *out;
      if (in[3] >= 128) {
        return (4);
      }
      *out = ((in[4] & 0x7FU) << 28) | *out;
      return (5);
    }

    // writes |value| to |p|
    int write_int(uint8_t *p, uint32_t value) const {
      if (value < (1U << 7)) {
        *p = static_cast<uint8_t>(value | (1U << 7));
        return (1);
      }
      else if (value < (1U << 14)) {
        *p = extract7bits<0>(value);
        ++p;
        *p = extract7bitsmaskless<1>(value) | (1U << 7);
        return (2);
      }
      else if (value < (1U << 21)) {
        *p = extract7bits<0>(value);
        ++p;
        *p = extract7bits<1>(value);
        ++p;
        *p = extract7bitsmaskless<2>(value) | (1U << 7);
        return (3);
      }
      else if (value < (1U << 28)) {
        *p = extract7bits<0>(value);
        ++p;
        *p = extract7bits<1>(value);
        ++p;
        *p = extract7bits<2>(value);
        ++p;
        *p = extract7bitsmaskless<3>(value) | (1U << 7);
        return (4);
      }
      else {
        *p = extract7bits<0>(value);
        ++p;
        *p = extract7bits<1>(value);
        ++p;
        *p = extract7bits<2>(value);
        ++p;
        *p = extract7bits<3>(value);
        ++p;
        *p = extract7bitsmaskless<4>(value) | (1U << 7);
        return (5);
      }
    }

    // returns the compressed size of |value|
    int calculate_delta_size(uint32_t value) {
      if (value < (1U << 7)) {
        return (1);
      }
      else if (value < (1U << 14)) {
        return (2);
      }
      else if (value < (1U << 21)) {
        return (3);
      }
      else if (value < (1U << 28)) {
        return (4);
      }
      else {
        return (5);
      }
    }

    template<uint32_t i>
    uint8_t extract7bits(const uint32_t val) const {
      return static_cast<uint8_t>((val >> (7 * i)) & ((1U << 7) - 1));
    }

    template<uint32_t i>
    uint8_t extract7bitsmaskless(const uint32_t val) const {
      return static_cast<uint8_t>((val >> (7 * i)));
    }

    // Returns a decompressed value
    uint32_t value(int slot) const {
      int position_in_block;
      Index *index = find_block_by_slot(slot, &position_in_block);

      ham_assert(position_in_block < index->key_count);
      ham_assert(index->key_count > 0);
      uint8_t *p = get_block_data(index);

      uint32_t prev = *(uint32_t *)p;
      p += sizeof(uint32_t);
      for (int i = 1; i <= position_in_block; i++) {
        uint32_t delta;
        p += read_int(p, &delta);
        prev += delta;
      }
      return (prev);
    }
    
    // fast-forwards to the specified position in a block
    uint8_t *fast_forward(Index *index, int position, uint32_t *pkey) {
      ham_assert(position > 0 && position <= index->key_count);
      uint8_t *p = get_block_data(index);
      uint32_t key = *(uint32_t *)p;
      p += sizeof(uint32_t);

      uint32_t delta;
      for (int i = 1; i < position; i++) {
        p += read_int(p, &delta);
        key += delta;
      }

      *pkey = key;
      return (p);
    }

    // Returns the payload data of a block
    uint8_t *get_block_data(Index *index) const {
      return (&m_data[8 + index->offset + sizeof(Index) * get_block_count()]);
    }

    // The persisted (compressed) data
    uint8_t *m_data;

    // The size of the persisted data
    size_t m_range_size;

    // helper variable to avoid returning pointers to local memory
    uint32_t m_dummy;
};

} // namespace Zint32

} // namespace hamsterdb

#endif /* HAM_BTREE_KEYS_ZINT32_H */
