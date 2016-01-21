/*
 * Copyright (C) 2005-2015 Christoph Rupp (chris@crupp.de).
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

import de.crupp.upscaledb.*;
import junit.framework.TestCase;

public class DatabaseTest extends TestCase {

  private class MyErrorHandler implements ErrorHandler {
    public int m_counter;

    public void handleMessage(int level, String message) {
      m_counter++;
    }
  }

  public void testSetErrorHandler() {
    Environment env = new Environment();
    MyErrorHandler eh = new MyErrorHandler();
    try {
      Database.setErrorHandler(eh);
      env.create(null);
    }
    catch (DatabaseException err) {
      assertEquals(Const.UPS_INV_PARAMETER, err.getErrno());
      assertEquals(1, eh.m_counter);
    }
    Database.setErrorHandler(null);
  }

  public void testGetVersion() {
    Version v = Database.getVersion();
    assertEquals(2, v.major);
    assertEquals(1, v.minor);
  }

  public void testDatabase() {
    Database db = new Database(0);
    db.close();
  }

  public void testGetDatabaseException() {
    Environment env = new Environment();
    try {
      env.create("jtest.db");
      Database db = env.createDatabase((short)1, 1234);
    }
    catch (DatabaseException err) {
      assertEquals(Const.UPS_INV_PARAMETER, err.getErrno());
    }
    env.close();
  }

  private class MyComparator implements CompareCallback
  {
    public int m_counter;

    public int compare(byte[] lhs, byte[] rhs) {
      m_counter++;
      return m_counter; /* need to return different values, or
                ups_insert thinks we're inserting
                duplicates */
    }
  }

  public void testSetComparator() {
    byte[] k = new byte[5];
    byte[] r = new byte[5];
    Environment env = new Environment();
    Database db;
    MyComparator cmp = new MyComparator();
    try {
      env.create("jtest.db");
      Parameter[] params = new Parameter[1];
      params[0] = new Parameter();
      params[0].name = Const.UPS_PARAM_KEY_TYPE;
      params[0].value = Const.UPS_TYPE_CUSTOM;
      db = env.createDatabase((short)1, 0, params);
      db.setComparator(cmp);
      db.insert(k, r);
      k[0] = 1;
      db.insert(k, r);
      k[0] = 2;
      db.insert(k, r);
      db.close();
    }
    catch (DatabaseException err) {
      fail("Exception " + err);
    }
    assertEquals(2, cmp.m_counter);
    env.close();
  }

  public void testGetParameters() {
    byte[] k = new byte[5];
    byte[] r = new byte[5];
    Parameter[] params = new Parameter[10];
    for (int i = 0; i < params.length; i++) {
      params[i] = new Parameter();
    }
    params[0].name = Const.UPS_PARAM_KEYSIZE;
    params[1].name = Const.UPS_PARAM_DATABASE_NAME;
    params[2].name = Const.UPS_PARAM_FLAGS;
    params[3].name = Const.UPS_PARAM_MAX_KEYS_PER_PAGE;
    Database db;
    Environment env = new Environment();
    try {
      env.create("jtest.db");
      db = env.createDatabase((short)1);
      db.getParameters(params);
      db.close();
    }
    catch (DatabaseException err) {
      fail("Exception " + err);
    }
    assertEquals(Const.UPS_KEY_SIZE_UNLIMITED, params[0].value);
    assertEquals(1, params[1].value);
    assertEquals(0, params[2].value);
    assertEquals(441, params[3].value);
    env.close();
  }

  public void testGetKeyCount() {
    byte[] k = new byte[5];
    byte[] r = new byte[5];
    Database db;
    Environment env = new Environment();
    try {
      env.create("jtest.db");
      db = env.createDatabase((short)1);
      assertEquals(0, db.getCount());
      db.insert(k, r);
      assertEquals(1, db.getCount());
      k[0] = 1;
      db.insert(k, r);
      assertEquals(2, db.getCount());
      k[0] = 2;
      db.insert(k, r);
      assertEquals(3, db.getCount());
      db.close();
    }
    catch (DatabaseException err) {
      fail("Exception "+err);
    }
    env.close();
  }
}
