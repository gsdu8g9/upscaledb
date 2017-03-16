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

/*
 * TODO
 * Add brief documentation about this file
 */

#ifndef UPS_TEMPLATE_H // TODO
#define UPS_TEMPLATE_H // TODO

#include "0root/root.h"

// TODO include c/c++ standard libraries
#include <ups/types.h>

// TODO include 3rd party headers
#include <json/json.h>

// TODO include public upscaledb headers
#include "ups/upscaledb.h"

// TODO include local headers, ordered by tier
// Always verify that a file of level N does not include headers > N!

#ifndef UPS_ROOT_H
#  error "root.h was not included"
#endif

namespace upscaledb {

struct Journal;
struct LocalEnv;

// Prefer struct over class
struct LocalEnvTest {
  LocalEnvTest(LocalEnv *env_)
    : env(env_), _journal(0) {
  }

  // Sets a new journal object
  void set_journal(Journal *journal);

  // NO leading underscore: public member. getters/setters are not
  // required
  LocalEnv *env;

  // leading underscore: "private" member
  Journal *_journal;
};

} // namespace upscaledb

#endif // UPS_TEMPLATE_H // TODO
