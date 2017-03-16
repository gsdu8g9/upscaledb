﻿/*
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

using System;

namespace Upscaledb
{
  /// <summary>
  /// A Transaction class
  /// </summary>
  public class Transaction : IDisposable
  {
    internal Transaction(Environment env, IntPtr handle) {
      this.env = env;
      this.handle = handle;
    }

    /// <summary>
    /// Commits the Transaction
    /// </summary>
    /// <remarks>
    /// This method wraps the native ups_txn_commit function.
    /// <br />
    /// Note that the function will fail with UPS_CURSOR_STILL_OPEN if
    /// a Cursor was attached to this Transaction, and the Cursor was
    /// not closed.
    /// </remarks>
    public void Commit() {
      int st;
      lock (env) {
        st = NativeMethods.TxnCommit(handle, 0);
      }
      if (st != 0)
        throw new DatabaseException(st);
      handle = IntPtr.Zero;
      env = null;
    }

    /// <summary>
    /// Aborts the Transaction if it has not already been committed or aborted.
    /// </summary>
    /// <remarks>
    /// This method wraps the native ups_txn_abort function.
    /// <br />
    /// Note that the function will fail with UPS_CURSOR_STILL_OPEN if
    /// a Cursor was attached to this Transaction, and the Cursor was
    /// not closed.
    /// </remarks>
    public void Abort() {
      if (env != null) {
        int st;
        lock (env) {
          st = NativeMethods.TxnAbort(handle, 0);
        }
        if (st != 0)
          throw new DatabaseException(st);
        handle = IntPtr.Zero;
        env = null;
      }
    }

    /// <summary>
    /// Aborts the Transaction if it has not already been committed or aborted.
    /// </summary>
    /// <see cref="Abort" />
    public void Dispose() {
      Abort();
    }

    /// <summary>
    /// Returns the low-level Transaction handle
    /// </summary>
    public IntPtr Handle {
      get {
        return handle;
      }
    }

    private Environment env;
    private IntPtr handle;
  }
}
