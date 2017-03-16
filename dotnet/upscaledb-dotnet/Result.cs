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

namespace Upscaledb
{
  using System;

  /// <summary>
  /// A UQI Result class
  /// </summary>
  public class Result : IDisposable
  {
    /// <summary>
    /// Constructor which creates a new Result
    /// </summary>
    /// <param name="handle">The internal upscaledb handle</param>
    internal Result(IntPtr handle) {
      this.handle = handle;
    }

    /// <summary>
    /// Closes the Cursor
    /// </summary>
    /// <remarks>
    /// This method wraps the native uqi_result_close function.
    /// <br />
    /// Closes this Result and frees allocated memory.
    /// </remarks>
    public void Close() {
      if (handle == IntPtr.Zero)
        return;
      NativeMethods.ResultClose(handle);
      handle = IntPtr.Zero;
    }

    /// <summary>
    /// Returns the number of rows.
    /// </summary>
    public int GetRowCount() {
      return NativeMethods.ResultGetRowCount(handle);
    }

    /// <summary>
    /// Returns the key type.
    /// </summary>
    public int GetKeyType() {
      return NativeMethods.ResultGetKeyType(handle);
    }

    /// <summary>
    /// Returns the record type.
    /// </summary>
    public int GetRecordType() {
      return NativeMethods.ResultGetRecordType(handle);
    }

    /// <summary>
    /// Returns the key of a specific row.
    /// </summary>
    public byte[] GetKey(int row) {
      return NativeMethods.ResultGetKey(handle, row);
    }

    // public extern void *ResultGetKeyData(IntPtr handle, ref int size);
    // TODO

    /// <summary>
    /// Returns the record of a specific row.
    /// </summary>
    public byte[] GetRecord(int row) {
      return NativeMethods.ResultGetRecord(handle, row);
    }

    // public void *ResultGetRecordData(IntPtr handle, ref int size);
    // TODO

    /// <summary>
    /// Closes the Result.
    /// </summary>
    /// <see cref="Close" />
    public void Dispose() {
      Close();
    }

    private IntPtr handle;
  }
}
