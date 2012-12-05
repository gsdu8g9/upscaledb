/**
 * Copyright (C) 2005-2012 Christoph Rupp (chris@crupp.de).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * See files COPYING.* for License information.
 *
 *
 * A simple example, which creates a database environment with
 * several databases.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* for exit() */
#include <ham/hamsterdb.h>

void
error(const char *foo, ham_status_t st) {
  printf("%s() returned error %d: %s\n", foo, st, ham_strerror(st));
  exit(-1);
}

#define MAX_DBS       2

#define DBNAME_CUSTOMER   1
#define DBNAME_ORDER    2

#define MAX_CUSTOMERS     4
#define MAX_ORDERS      8

/* A structure for the "customer" database */
typedef struct {
  int id;         /* customer id */
  char name[32];      /* customer name */
  /* ... additional information could follow here */
} customer_t;

/* A structure for the "orders" database */
typedef struct {
  int id;         /* order id */
  int customer_id;    /* customer id */
  char assignee[32];    /* assigned to whom? */
  /* ... additional information could follow here */
} order_t;

int
main(int argc, char **argv) {
  int i;
  ham_status_t st;    /* status variable */
  ham_db_t *db[MAX_DBS];  /* hamsterdb database objects */
  ham_env_t *env;     /* hamsterdb environment */
  ham_cursor_t *cursor[MAX_DBS]; /* a cursor for each database */
  ham_key_t key, cust_key, ord_key;
  ham_record_t record, cust_record, ord_record;

  customer_t customers[MAX_CUSTOMERS] = {
    { 1, "Alan Antonov Corp." },
    { 2, "Barry Broke Inc." },
    { 3, "Carl Caesar Lat." },
    { 4, "Doris Dove Brd." }
  };

  order_t orders[MAX_ORDERS] = {
    { 1, 1, "Joe" },
    { 2, 1, "Tom" },
    { 3, 3, "Joe" },
    { 4, 4, "Tom" },
    { 5, 3, "Ben" },
    { 6, 3, "Ben" },
    { 7, 4, "Chris" },
    { 8, 1, "Ben" }
  };

  memset(&key, 0, sizeof(key));
  memset(&record, 0, sizeof(record));
  memset(&cust_key, 0, sizeof(cust_key));
  memset(&cust_record, 0, sizeof(cust_record));
  memset(&ord_key, 0, sizeof(ord_key));
  memset(&ord_record, 0, sizeof(ord_record));

  /* Now create a new hamsterdb Environment */
  st = ham_env_create(&env, "test.db", 0, 0664, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_create", st);

  /*
   * Then create the two Databases in this Environment; each Database
   * has a name - the first is our "customer" Database, the second
   * is for the "orders"
   */
  st = ham_env_create_db(env, &db[0], DBNAME_CUSTOMER, 0, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_create_db (customer)", st);
  st = ham_env_create_db(env, &db[1], DBNAME_ORDER, 0, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_create_db (order)", st);

  /* Create a Cursor for each Database */
  for (i = 0; i < MAX_DBS; i++) {
    st = ham_cursor_create(db[i], 0, 0, &cursor[i]);
    if (st != HAM_SUCCESS) {
      printf("ham_cursor_create() failed with error %d\n", st);
      return (-1);
    }
  }

  /* Insert a few customers in the first database */
  for (i = 0; i < MAX_CUSTOMERS; i++) {
    key.size = sizeof(int);
    key.data = &customers[i].id;

    record.size = sizeof(customer_t);
    record.data = &customers[i];

    st = ham_db_insert(db[0], 0, &key, &record, 0);
    if (st != HAM_SUCCESS)
      error("ham_db_insert (customer)", st);
  }

  /* And now the orders in the second database */
  for (i = 0; i < MAX_ORDERS; i++) {
    key.size = sizeof(int);
    key.data = &orders[i].id;

    record.size = sizeof(order_t);
    record.data = &orders[i];

    st = ham_db_insert(db[1], 0, &key, &record, 0);
    if (st != HAM_SUCCESS)
      error("ham_db_insert (order)", st);
  }

  /*
   * To demonstrate even more functions: close all objects, then
   * re-open the environment and the two databases.
   *
   * Note that ham_env_close automatically calls ham_db_close on all
   * databases.
   */
  for (i = 0; i < MAX_DBS; i++) {
    st = ham_cursor_close(cursor[i]);
    if (st != HAM_SUCCESS)
      error("ham_cursor_close", st);
  }
  st = ham_env_close(env, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_close", st);

  /* Now reopen the environment and the databases */
  st = ham_env_open(&env, "test.db", 0, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_open", st);
  st = ham_env_open_db(env, &db[0], DBNAME_CUSTOMER, 0, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_open_db (customer)", st);
  st = ham_env_open_db(env, &db[1], DBNAME_ORDER, 0, 0);
  if (st != HAM_SUCCESS)
    error("ham_env_open_db (order)", st);

  /* Re-create a cursor for each database */
  for (i = 0; i < MAX_DBS; i++) {
    st = ham_cursor_create(db[i], 0, 0, &cursor[i]);
    if (st != HAM_SUCCESS) {
      printf("ham_cursor_create() failed with error %d\n", st);
      return (-1);
    }
  }

  /*
   * Now start the query - we want to dump each customer with his
   * orders
   *
   * We have a loop with two cursors - the first cursor looping over
   * the database with customers, the second loops over the orders.
   */
  while (1) {
    customer_t *customer;

    st = ham_cursor_move(cursor[0], &cust_key, &cust_record,
            HAM_CURSOR_NEXT);
    if (st != HAM_SUCCESS) {
      /* reached end of the database? */
      if (st == HAM_KEY_NOT_FOUND)
        break;
      else
        error("ham_cursor_next(customer)", st);
    }

    customer = (customer_t *)cust_record.data;

    /* print the customer id and name */
    printf("customer %d ('%s')\n", customer->id, customer->name);

    /*
     * The inner loop prints all orders of this customer. Move the
     * cursor to the first entry.
     */
    st = ham_cursor_move(cursor[1], &ord_key, &ord_record,
            HAM_CURSOR_FIRST);
    if (st != HAM_SUCCESS) {
      /* reached end of the database? */
      if (st == HAM_KEY_NOT_FOUND)
        continue;
      else
        error("ham_cursor_next(order)", st);
    }

    do {
      order_t *order = (order_t *)ord_record.data;

      /* print this order, if it belongs to the current customer */
      if (order->customer_id == customer->id)
        printf("  order: %d (assigned to %s)\n",
            order->id, order->assignee);

      st = ham_cursor_move(cursor[1], &ord_key,
          &ord_record, HAM_CURSOR_NEXT);
      if (st != HAM_SUCCESS) {
        /* reached end of the database? */
        if (st == HAM_KEY_NOT_FOUND)
          break;
        else
          error("ham_cursor_next(order)", st);
      }
    } while (1);
  }

  /*
   * Now close the environment handle; the flag HAM_AUTO_CLEANUP will
   * automatically close all databases and cursors
   */
  st = ham_env_close(env, HAM_AUTO_CLEANUP);
  if (st != HAM_SUCCESS)
    error("ham_env_close", st);

  printf("success!\n");
  return (0);
}

