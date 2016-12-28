/*
 * Copyright (C) 2005-2016 Christoph Rupp (chris@crupp.de).
 * All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See the file COPYING for License information.
 */

/*
 * The PageManager allocates, fetches and frees pages. It manages the
 * list of all pages (free and not free), and maps their virtual ID to
 * their physical address in the file.
 *
 * @exception_safe: basic
 * @thread_safe: yes
 */

#ifndef UPS_PAGE_MANAGER_H
#define UPS_PAGE_MANAGER_H

// include this first, otherwise WIN32 compilation (boost/asio.hpp) fails
#include "2worker/worker.h"

#include "0root/root.h"

#include <map>

// Always verify that a file of level N does not include headers > N!
#include "1base/scoped_ptr.h"
#include "3page_manager/page_manager_state.h"

#ifndef UPS_ROOT_H
#  error "root.h was not included"
#endif

namespace upscaledb {

struct Context;
struct LocalDb;
struct LocalEnv;

struct PageManager
{
  enum {
    // flag for alloc(): Clear the full page with zeroes
    kClearWithZero     = 1,

    // flag for alloc(): Ignores the freelist
    kIgnoreFreelist    = 2,

    // flag for alloc(): Do not persist the PageManager state to disk
    kDisableStoreState = 4,

    // Flag for fetch(): only fetches from cache, not from disk
    kOnlyFromCache = 1,

    // Flag for fetch(): does not add page to the Changeset
    kReadOnly = 2,

    // Flag for fetch(): page is part of a multi-page blob, has no header
    kNoHeader = 4
  };

  // Constructor
  PageManager(LocalEnv *env)
    : state(new PageManagerState(env)) {
  }

  // Loads the state from a blob
  void initialize(uint64_t blobid);

  // Fills in the current metrics for the PageManager, the Cache and the
  // Freelist
  void fill_metrics(ups_env_metrics_t *metrics) const;

  // Fetches a page from disk. |flags| are bitwise OR'd: kOnlyFromCache,
  // kReadOnly, kNoHeader...
  // The page is locked and stored in |context->changeset|.
  Page *fetch(Context *context, uint64_t address, uint32_t flags = 0);

  // Allocates a new page. |page_type| is one of Page::kType* in page.h.
  // |flags| are either 0 or kClearWithZero
  // The page is locked and stored in |context->changeset|.
  Page *alloc(Context *context, uint32_t page_type, uint32_t flags = 0);

  // Allocates multiple adjacent pages.
  // Used by the BlobManager to store blobs that span multiple pages
  // Returns the first page in the list of pages
  // The pages are locked and stored in |context->changeset|.
  Page *alloc_multiple_blob_pages(Context *context, size_t num_pages);

  // Flushes all pages to disk
  void flush_all_pages();

  // Asks the worker thread to purge the cache if the cache limits are
  // exceeded
  void purge_cache(Context *context);

  // Reclaim file space; truncates unused file space at the end of the file.
  void reclaim_space(Context *context);

  // Flushes and closes all pages of a database
  void close_database(Context *context, LocalDb *db);

  // Schedules one (or many sequential) pages for deletion and adds them
  // to the Freelist
  void del(Context *context, Page *page, size_t page_count = 1);

  // Closes the PageManager; flushes all dirty pages
  void close(Context *context);

  // Calls close(), then re-initializes the PageManager; used to restart
  // the internal state after recovery was performed
  void reset(Context *context);

  // Returns the Page pointer where we can add more blobs
  Page *last_blob_page(Context *context);

  // Sets the Page pointer where we can add more blobs
  void set_last_blob_page(Page *page);

  // Returns the id of the Page where we can add more blobs. Required by
  // the BlobManager
  uint64_t last_blob_page_id();

  // Sets the id of the Page where we can add more blobs.
  // Required by the BlobManager.
  void set_last_blob_page_id(uint64_t id);

  // Fetches a page from the cache, locks it, then returns it.
  // This method is used by the worker thread to fetch purge candidates.
  // Returns NULL if the page cannot be purged (i.e. because it cannot
  // be locked or cursors are attached) 
  Page *try_lock_purge_candidate(uint64_t page_id);

  // Adds a message to the worker's queue
  template<typename WorkerMessage>
  void run_async(WorkerMessage message) {
    return state->worker->enqueue(message);
  }

  // Stores the state to disk. Returns the page-Id with the persisted state.
  // Exposed here because it's required by the unittests.
  uint64_t test_store_state();

  // The state
  ScopedPtr<PageManagerState> state;
};

} // namespace upscaledb

#endif /* UPS_PAGE_MANAGER_H */
