/*
 * Copyright (C) 2005-2017 Christoph Rupp (chris@crupp.de).
 * All Rights Reserved.
 *
 * This program is released under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * See the file COPYING for License information.
 */

#ifndef UPS_BTREE_NODE_H
#define UPS_BTREE_NODE_H

#include "0root/root.h"

// Always verify that a file of level N does not include headers > N!
#include "2page/page.h"
#include "3btree/btree_flags.h"

#ifndef UPS_ROOT_H
#  error "root.h was not included"
#endif

namespace upscaledb {

class PBtreeKeyDefault;

#include "1base/packstart.h"

/*
 * A BtreeNode structure spans the persistent part of a Page
 *
 * This structure is directly written to/read from the file.
 */
UPS_PACK_0 struct UPS_PACK_1 PBtreeNode {
  public:
    // Result of the insert() operation
    struct InsertResult {
      InsertResult(ups_status_t _status = 0, int _slot = 0)
        : status(_status), slot(_slot) {
      }

      // upscaledb status code
      ups_status_t status;

      // the slot of the new (or existing) key
      int slot;
    };

    enum {
      // insert key at the beginning of the page
      kInsertPrepend = 1,

      // append key to the end of the page
      kInsertAppend = 2,
    };

    enum {
      // node is a leaf
      kLeafNode = 1
    };

    // Returns a PBtreeNode from a Page
    static PBtreeNode *from_page(Page *page) {
      return (PBtreeNode *)page->payload();
    }

    // Returns the offset (in bytes) of the member |m_data|
    static uint32_t entry_offset() {
      return sizeof(PBtreeNode) - 1;
    }

    // Returns the flags of the btree node (|kLeafNode|)
    uint32_t flags() const {
      return _flags;
    }

    // Sets the flags of the btree node (|kLeafNode|)
    void set_flags(uint32_t flags) {
      _flags = flags;
    }

    // Returns the number of entries in a BtreeNode
    uint32_t length() const {
      return _length;
    }

    // Sets the number of entries in a BtreeNode
    void set_length(uint32_t length) {
      _length = length;
    }

    // Returns the address of the left sibling of this node
    uint64_t left_sibling() const {
      return _left_sibling;
    }

    // Sets the address of the left sibling of this node
    void set_left_sibling(uint64_t left) {
      _left_sibling = left;
    }

    // Returns the address of the right sibling of this node
    uint64_t right_sibling() const {
      return _right_sibling;
    }

    // Sets the address of the right sibling of this node
    void set_right_sibling(uint64_t right) {
      _right_sibling = right;
    }

    // Returns the left child pointer of this node
    uint64_t left_child() const {
      return _left_child;
    }

    // Sets the ptr_down of this node
    void set_left_child(uint64_t left_child) {
      _left_child = left_child;
    }

    // Returns true if this btree node is a leaf node
    bool is_leaf() const {
      return ISSET(_flags, kLeafNode);
    }

    // Returns a pointer to the key data
    uint8_t *data() {
      return &_data[0];
    }

    const uint8_t *data() const {
      return &_data[0];
    }

  private:
    // flags of this node
    uint32_t _flags;

    // number of used entries in the node
    uint32_t _length;
  
    // address of left sibling
    uint64_t _left_sibling;

    // address of right sibling
    uint64_t _right_sibling;

    // address of child node whose items are smaller than all items
    // in this node
    uint64_t _left_child;

    // the entries of this node
    uint8_t _data[1];

} UPS_PACK_2;

#include "1base/packstop.h"

} // namespace upscaledb

#endif // UPS_BTREE_NODE_H
