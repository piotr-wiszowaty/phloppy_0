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

import java.io.IOException;
import java.net.Socket;
import java.util.Arrays;
import java.util.Queue;

class Receiver extends Thread {
    private Socket socket;
    private Queue<Message> messages;

    Receiver(Socket socket, Queue<Message> messages) {
        this.socket = socket;
        this.messages = messages;
    }

    void close() {
        interrupt();
    }

    @Override
    public void run() {
        while (true) {
            try {
                byte[] b = new byte[4096];
                int length = socket.getInputStream().read(b);
                if (length > 0) {
                    messages.offer(new Message(Message.Command.RX, Arrays.copyOf(b, length)));
                } else if (length < 0) {
                    break;
                }
            } catch (IOException e) {
                break;
            }
        }
    }
}
