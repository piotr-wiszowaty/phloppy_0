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

class Message {
    enum Command {
        EXIT,
        INSERT,
        EJECT,
        WRITE_PROTECT,
        WRITE_UNPROTECT,
        RX
    }

    Command command;
    int drvno = -1;
    byte[] data;

    Message(Command command, int drvno, byte[] data) {
        this.command = command;
        this.drvno = drvno;
        this.data = data;
    }

    Message(Command command, byte[] data) {
        this.command = command;
        this.data = data;
    }

    Message(Command command, int drvno) {
        this.command = command;
        this.drvno = drvno;
    }

    Message(Command command) {
        this.command = command;
    }

    @Override
    public String toString() {
        StringBuilder b = new StringBuilder();
        b.append("Message[cmd:");
        b.append(command);
        b.append(",drvno:");
        b.append(drvno);
        b.append(",data:");
        if (data != null) {
            for (int i = 0; i < Math.min(64, data.length); i++) {
                if (i > 0) {
                    b.append(',');
                }
                b.append(String.format("%02x", 0xff & data[i]));
            }
        }
        b.append("]");
        return b.toString();
    }
}
