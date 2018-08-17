/*
 * phloppy_0 - Commodore Amiga floppy drive emulator
 * Copyright (C) 2016-2018 Piotr Wiszowaty
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package pwi.phloppy_0;

import android.util.Log;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

public class FloppyImage {
    private static final String TAG = "Phloppy_0/ADF";

    static final int TRACKS_PER_DISK = 2 * 80;
    static final int ADF_BYTES_PER_TRACK = 512 * 11;
    static final int RAW_BYTES_PER_TRACK = 12668;

    private String path;
    private byte[] id;
    private RandomAccessFile file;
    private boolean raw;
    private int bytesPerTrack;

    FloppyImage(String path) throws Exception {
        this.path = path;
        file = new RandomAccessFile(path, "rw");
        byte[] data = new byte[(int) file.length()];
        file.read(data);

        MessageDigest md = MessageDigest.getInstance("SHA1");
        id = md.digest(data);

        if (file.length() == ADF_BYTES_PER_TRACK * TRACKS_PER_DISK) {
            bytesPerTrack = ADF_BYTES_PER_TRACK;
            raw = false;
        } else if (file.length() == RAW_BYTES_PER_TRACK * TRACKS_PER_DISK) {
            bytesPerTrack = RAW_BYTES_PER_TRACK;
            raw = true;
        } else {
            throw new Exception("File size not handled: " + file.length());
        }
    }

    byte[] id() {
        return Arrays.copyOf(id, id.length);
    }

    boolean raw() {
        return this.raw;
    }

    int bytesPerTrack() {
        return bytesPerTrack;
    }

    byte[] data() throws IOException {
        file.seek(0);
        byte b[] = new byte[(int) file.length()];
        file.read(b);
        return b;
    }

    void close() throws IOException {
        Log.d(TAG, "Closing " + path);
        file.close();
    }

    void write(byte[] b, int offset) throws IOException {
        int length = bytesPerTrack();
        Log.d(TAG, "Writing " + path + ": " + offset + ".." + (offset + length));
        file.seek(offset);
        file.write(b, 0, length);
    }
}
