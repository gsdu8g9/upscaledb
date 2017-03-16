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

/**
 * A simple example which creates a server with one Environment and
 * several Databases. See client1.c for the corresponding client.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ups/upscaledb.h>
#include <ups/upscaledb_srv.h>

#ifdef WIN32
#   define EXT ".exe"
#else
#   define EXT ""
#endif

int
main()
{
  ups_db_t *db;
  ups_env_t *env;
  ups_srv_t *srv;
  ups_srv_config_t cfg;
  ups_status_t st;
  char input[1024];
  int s;

  /* create a new Environment; this Environment will be attached to the
   * server */
  st = ups_env_create(&env, "env1.db", UPS_ENABLE_TRANSACTIONS, 0644, 0);
  if (st) {
    printf("ups_env_create: %d\n", st);
    exit(-1);
  }

  /* also create a Database in that Environment ... */
  st = ups_env_create_db(env, &db, 12, UPS_ENABLE_DUPLICATE_KEYS, 0);
  if (st) {
    printf("ups_env_create_db: %d\n", st);
    exit(-1);
  }

  /* ... and close it again. It will be reopened remotely. */
  ups_db_close(db, 0);

  /* Create a second database */
  st = ups_env_create_db(env, &db, 13, UPS_ENABLE_DUPLICATE_KEYS, 0);
  if (st) {
    printf("ups_env_create_db: %d\n", st);
    exit(-1);
  }

  ups_db_close(db, 0);

  st = ups_env_create_db(env, &db, 33,
                UPS_RECORD_NUMBER64 | UPS_ENABLE_DUPLICATE_KEYS, 0);
  if (st) {
    printf("ups_env_create_db: %d\n", st);
    exit(-1);
  }

  ups_db_close(db, 0);

  /* The ups_srv_config_t structure describes the settings of the server
   * including the port, the Environment etc */
  memset(&cfg, 0, sizeof(cfg));
  cfg.port = 8080;
  ups_srv_init(&cfg, &srv);
  ups_srv_add_env(srv, env, "/env1.db");

  printf("server1%s started - please run sample 'client1%s' for a test\n",
      EXT, EXT);
  printf("type 'exit' to end the server\n");

  /* See client1.c for the corresponding client */
  while (1) {
    printf("> ");
    s = scanf("%s", &input[0]);
    if (s == EOF || !strcmp(input, "exit")) {
      printf("exiting...\n");
      break;
    }
    printf("unknown command\n");
  }

  /* Close the server and the Environment */
  ups_srv_close(srv);
  ups_env_close(env, UPS_AUTO_CLEANUP);

  return (0);
}
