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


#include <cstdio>
#include <cstring>

#include "ups/upscaledb.h"
#include "ups/upscaledb_uqi.h"

extern "C" {

static uqi_plugin_t plugin1 = {0};
static uqi_plugin_t plugin2 = {0};
static uqi_plugin_t plugin3 = {0};
static uqi_plugin_t plugin4 = {0};

static int
predicate_function(void *state, const void *key_data, uint32_t key_size,
                const void *record_data, uint32_t record_size)
{
  return false;
}

UPS_EXPORT
uqi_plugin_t * UPS_CALLCONV
plugin_descriptor(const char *name)
{
  if (!::strcmp(name, "test1")) {
    plugin1.name = "test1";
    plugin1.plugin_version = 99999;
    return &plugin1;
  }

  if (!::strcmp(name, "test2")) {
    plugin2.name = "test2";
    return &plugin2;
  }

  if (!::strcmp(name, "test3")) {
    plugin3.name = "test3";
    plugin3.plugin_version = 0;
    plugin3.type = UQI_PLUGIN_PREDICATE;
    return &plugin3;
  }

  if (!::strcmp(name, "test4")) {
    plugin4.name = "test4";
    plugin4.plugin_version = 0;
    plugin4.type = UQI_PLUGIN_PREDICATE;
    plugin4.pred = predicate_function;
    return &plugin4;
  }

  return 0;
}

}
