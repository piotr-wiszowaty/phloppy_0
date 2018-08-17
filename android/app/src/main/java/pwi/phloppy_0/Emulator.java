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

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import pwi.phloppy_0.Message.Command;

import static java.lang.Math.min;

class Emulator extends Thread {
    public interface Callback {
        void onEmulatorConnected(boolean success, String errorMessage);

        void onImageLoaded(int drvno, boolean success, String errorMessage);

        void onTrackWritten(int drvno, int tt);

        void onWriteError(int drvno, String errorMessage);
    }

    private static final String TAG = "Phloppy_0/Emulator";

    private static final byte END = (byte) 0xc0;
    private static final byte ESC = (byte) 0xdb;
    private static final byte ESC_END = (byte) 0xdc;
    private static final byte ESC_ESC = (byte) 0xdd;

    private static final int RX_ERROR = -1;
    private static final int RX_END = -0xc0;
    private static final int RX_ESC = -0xdb;

    private static final byte OP_NOP = 0x00;
    private static final byte OP_INSERT0 = 0x01;
    private static final byte OP_INSERT1 = 0x02;
    private static final byte OP_EJECT0 = 0x03;
    private static final byte OP_EJECT1 = 0x04;
    private static final byte OP_FILL0 = 0x05;
    private static final byte OP_FILL1 = 0x06;
    private static final byte OP_WPROT0 = 0x07;
    private static final byte OP_WPROT1 = 0x08;
    private static final byte OP_WUNPROT0 = 0x09;
    private static final byte OP_WUNPROT1 = 0x0a;
    private static final byte OP_INSERT2 = 0x11;
    private static final byte OP_INSERT3 = 0x12;
    private static final byte OP_EJECT2 = 0x13;
    private static final byte OP_EJECT3 = 0x14;
    private static final byte OP_FILL2 = 0x15;
    private static final byte OP_FILL3 = 0x16;
    private static final byte OP_WPROT2 = 0x17;
    private static final byte OP_WPROT3 = 0x18;
    private static final byte OP_WUNPROT2 = 0x19;
    private static final byte OP_WUNPROT3 = 0x1a;
    private static final byte OP_TYPE0_ADF = 0x1b;
    private static final byte OP_TYPE1_ADF = 0x1c;
    private static final byte OP_TYPE2_ADF = 0x1d;
    private static final byte OP_TYPE3_ADF = 0x1e;
    private static final byte OP_TYPE0_RAW = 0x1f;
    private static final byte OP_TYPE1_RAW = 0x20;
    private static final byte OP_TYPE2_RAW = 0x21;
    private static final byte OP_TYPE3_RAW = 0x22;

    private static final int ROOTBLOCK_OFFSET = 0x6e000;
    private static final int ROOTBLOCK_CKSUM_OFFSET = 0x6e014;
    private static final int DISKNAME_LENGTH_OFFSET = 0x6e1b0;
    private static final int DISKNAME_OFFSET = 0x6e1b1;

    private static final Object[] emptyADFImage = {
            new byte[] {68, 79, 83},
            450560,
            new byte[] {2},
            11,
            new byte[] {72},
            4,
            new byte[] {(byte) -122, 32, (byte) -56, (byte) -43},
            288,
            new byte[] {(byte) -1, (byte) -1, (byte) -1, (byte) -1},
            2,
            new byte[] {3, 113},
            102,
            new byte[] {50, 18},
            2,
            new byte[] {5, 48},
            2,
            new byte[] {10, (byte) -66},
            1, 30,  // diskname length, diskname
            11,
            new byte[] {50, 18},
            2,
            new byte[] {5, 48},
            2,
            new byte[] {10, (byte) -66},
            2,
            new byte[] {50, 18},
            2,
            new byte[] {5, 48},
            2,
            new byte[] {10, (byte) -66},
            15,
            new byte[] {1},
            2,
            new byte[] {(byte) -64, 55, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, 63, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1, (byte) -1},
            449824
    };

    private enum RxState {
        IDLE,
        DRVNO,
        TT,
        TRANSMIT
    }

    private Callback callback;
    private String host;
    private int port;

    private Socket mainSocket;
    private BlockingQueue<Message> messages = new LinkedBlockingQueue<>();
    private Receiver receiver;
    private RxState rxState = RxState.IDLE;
    private FloppyImage[] images = {null, null, null, null};
    private boolean[] writeProtection = {true, true, true, true};
    private byte[] padding;

    private int rxDrvno;
    private int rxOffset;
    private int rxCount;
    private byte[] rxTrack = new byte[16384];
    private boolean slipEscaping = false;

    public Emulator(String host, int port, Callback callback) {
        this.host = host;
        this.port = port;
        this.callback = callback;

        padding = new byte[65];
        padding[0] = END;
        padding[1] = OP_NOP;
    }

    public void close() {
        interrupt();
        try {
            messages.put(new Message(Command.EXIT, -1, null));
        } catch (InterruptedException e) {
        }
    }

    public void insert(int drvno, String path) {
        try {
            messages.put(new Message(Command.INSERT, drvno, path.getBytes()));
        } catch (InterruptedException e) {
        }
    }

    public void eject(int drvno) {
        try {
            messages.put(new Message(Command.EJECT, drvno));
        } catch (InterruptedException e) {
        }
    }

    public void writeProtect(int drvno, boolean flag) {
        try {
            if (flag) {
                messages.put(new Message(Command.WRITE_PROTECT, drvno));
            } else {
                messages.put(new Message(Command.WRITE_UNPROTECT, drvno));
            }
        } catch (InterruptedException e) {
        }
    }

    public void run() {
        try {
            mainSocket = new Socket(host, port);
            mainSocket.setKeepAlive(true);
            Log.d(TAG, "Connected to " + host + ":" + port);
            receiver = new Receiver(mainSocket, messages);
            receiver.start();
            callback.onEmulatorConnected(true, null);
        } catch (IOException e) {
            callback.onEmulatorConnected(false, e.getMessage());
            return;
        }

        while (true) {
            try {
                Message message;
                if (rxState != RxState.IDLE) {
                    message = messages.poll();
                } else {
                    message = messages.poll(250, TimeUnit.MILLISECONDS);
                }
                if (message == null) {
                    // Force rx
                    if (rxState != RxState.IDLE) {
                        send(new byte[]{END, OP_NOP}, new byte[4096 - 2]);
                    } else {
                        send(new byte[]{END, OP_NOP});
                    }
                } else {
                    Log.d(TAG, message.toString());
                    switch (message.command) {
                        case INSERT:
                            closeImage(message.drvno);
                            openImage(message.drvno, new String(message.data));
                            break;

                        case EJECT:
                            sendEject(message.drvno);
                            closeImage(message.drvno);
                            break;

                        case WRITE_PROTECT:
                            writeProtection[message.drvno] = true;
                            sendWriteProtect(message.drvno);
                            break;

                        case WRITE_UNPROTECT:
                            writeProtection[message.drvno] = false;
                            sendWriteProtect(message.drvno);
                            break;

                        case RX:
                            processRX(message.data);
                            Log.d(TAG, "Total rx bytes: " + rxCount + ", state: " + rxState);
                            break;
                    }
                }
            } catch (InterruptedException e) {
                Log.d(TAG, e.getMessage(), e);
                break;
            } catch (IOException e) {
                Log.d(TAG, e.getMessage(), e);
                break;
            }
        }
        Log.d(TAG, "Closing connections");

        receiver.close();
        try {
            mainSocket.close();
        } catch (IOException e) {
        }

        closeImage(0);
        closeImage(1);
        closeImage(2);
        closeImage(3);

        Log.d(TAG, "Emulator finished");
    }

    private byte[][] readRemoteIds(Socket auxSocket) throws IOException {
        byte[][] buffer = {new byte[20], new byte[20], new byte[20], new byte[20]};
        for (int k = 0; k < 4; k++) {
            for (int n = 0; n < 20; ) {
                n += auxSocket.getInputStream().read(buffer[k], n, buffer[k].length - n);
            }
            Log.d(TAG, "Remote ID " + k + ": " + idToString(buffer[k]));
        }
        return buffer;
    }

    private byte[] getRemoteId(int drvno) throws IOException {
        Socket auxSocket = new Socket(host, port + 1);
        Log.d(TAG, "Connected to " + host + ":" + (port + 1));
        byte[][] ids = readRemoteIds(auxSocket);
        auxSocket.close();
        return ids[drvno];
    }

    private void setRemoteId(int drvno, byte[] id) throws IOException {
        Socket auxSocket = new Socket(host, port + 1);
        Log.d(TAG, "Connected to " + host + ":" + (port + 1));
        byte[][] ids = readRemoteIds(auxSocket);
        ids[drvno] = id;
        for (int i = 0; i < 4; i++) {
            auxSocket.getOutputStream().write(ids[i]);
        }
        auxSocket.close();
    }

    private void openImage(int drvno, String path) {
        try {
            long t0 = System.currentTimeMillis();
            FloppyImage img = images[drvno] = new FloppyImage(path);
            byte[] remoteId = getRemoteId(drvno);
            if (!Arrays.equals(img.id(), remoteId)) {
                Log.d(TAG, "Sending drive " + drvno + " data");
                if (drvno == 0) {
                    byte type = img.raw() ? OP_TYPE0_RAW : OP_TYPE0_ADF;
                    send(new byte[]{END, OP_EJECT0, END, type, END, OP_FILL0}, slipEncode(img.data()), new byte[]{END, OP_INSERT0});
                    setRemoteId(drvno, img.id());
                } else if (drvno == 1) {
                    byte type = img.raw() ? OP_TYPE1_RAW : OP_TYPE1_ADF;
                    send(new byte[]{END, OP_EJECT1, END, type, END, OP_FILL1}, slipEncode(img.data()), new byte[]{END, OP_INSERT1});
                    setRemoteId(drvno, img.id());
                } else if (drvno == 2) {
                    byte type = img.raw() ? OP_TYPE2_RAW : OP_TYPE2_ADF;
                    send(new byte[]{END, OP_EJECT2, END, type, END, OP_FILL2}, slipEncode(img.data()), new byte[]{END, OP_INSERT2});
                    setRemoteId(drvno, img.id());
                } else if (drvno == 3) {
                    byte type = img.raw() ? OP_TYPE3_RAW : OP_TYPE3_ADF;
                    send(new byte[]{END, OP_EJECT3, END, type, END, OP_FILL3}, slipEncode(img.data()), new byte[]{END, OP_INSERT3});
                    setRemoteId(drvno, img.id());
                }
                long t1 = System.currentTimeMillis();
                Log.d(TAG, "Elapsed time: " + (t1 - t0) + " ms");
            } else {
                Log.d(TAG, "Skip sending drive " + drvno + " data");
            }

            callback.onImageLoaded(drvno, true, null);
        } catch (Exception e) {
            callback.onImageLoaded(drvno, false, e.getMessage());
        }
    }

    private void closeImage(int drvno) {
        if (images[drvno] != null) {
            try {
                Log.d(TAG, "Closing image " + drvno);
                images[drvno].close();
            } catch (IOException e) {
            }
            images[drvno] = null;
        }
    }

    private void sendEject(int drvno) throws IOException {
        if (drvno == 0) {
            setRemoteId(drvno, new byte[20]);
            send(OP_EJECT0);
        } else if (drvno == 1) {
            setRemoteId(drvno, new byte[20]);
            send(OP_EJECT1);
        } else if (drvno == 2) {
            setRemoteId(drvno, new byte[20]);
            send(OP_EJECT2);
        } else if (drvno == 3) {
            setRemoteId(drvno, new byte[20]);
            send(OP_EJECT3);
        }
        Log.d(TAG, "Send IDs (2)");
    }

    private void sendWriteProtect(int drvno) throws IOException {
        if (drvno == 0) {
            if (writeProtection[drvno]) {
                send(OP_WPROT0);
            } else {
                send(OP_WUNPROT0);
            }
        } else if (drvno == 1) {
            if (writeProtection[drvno]) {
                send(OP_WPROT1);
            } else {
                send(OP_WUNPROT1);
            }
        } else if (drvno == 2) {
            if (writeProtection[drvno]) {
                send(OP_WPROT2);
            } else {
                send(OP_WUNPROT2);
            }
        } else if (drvno == 3) {
            if (writeProtection[drvno]) {
                send(OP_WPROT3);
            } else {
                send(OP_WUNPROT3);
            }
        }
    }

    private void send(byte b) throws IOException {
        send(new byte[]{END, b});
    }

    private void send(byte[]... data) throws IOException {
        int total = 0;
        for (byte[] d : data) {
            mainSocket.getOutputStream().write(d);
            total += d.length;
        }

        int length = 64 - total % 64;
        if (length == 1) {
            mainSocket.getOutputStream().write(padding, 0, 65);
        } else if (length > 0) {
            mainSocket.getOutputStream().write(padding, 0, length);
        }
    }

    private void processRX(byte[] bytes) {
        for (byte b : bytes) {
            int d = slipDecode(b);
            //Log.d(TAG, String.format("processRX: %s $%03x $%02x/%d %d", rxState.toString(), rxCount, (0xff & b), (0xff & b), d));
            if (rxState == RxState.IDLE) {
                if (d == RX_END) {
                    rxState = RxState.DRVNO;
                }
            } else if (rxState == RxState.DRVNO) {
                if (d == RX_ESC) {
                    // pass
                } else if (d == RX_END || d == RX_ERROR) {
                    rxState = RxState.IDLE;
                    Log.d(TAG, "Exit DRVNO: byte=" + d + ", cnt=" + rxCount);
                } else {
                    rxState = RxState.TT;
                    rxDrvno = d;
                    if (writeProtection[rxDrvno]) {
                        rxState = RxState.IDLE;
                    }
                }
            } else if (rxState == RxState.TT) {
                if (d == RX_ESC) {
                    // pass
                } else if (d == RX_END || d == RX_ERROR) {
                    rxState = RxState.IDLE;
                    Log.d(TAG, "Exit TT: byte=" + d + ", cnt=" + rxCount);
                } else {
                    rxState = RxState.TRANSMIT;
                    rxOffset = d * images[rxDrvno].bytesPerTrack();
                    rxCount = 0;
                }
            } else if (rxState == RxState.TRANSMIT) {
                if (d == RX_ESC) {
                    // pass
                } else if (d == RX_END || d == RX_ERROR) {
                    rxState = RxState.IDLE;
                    Log.d(TAG, "Exit TRANSMIT: byte=" + d + ", cnt=" + rxCount + ", bytes/track=" + images[rxDrvno].bytesPerTrack());
                } else {
                    rxTrack[rxCount++] = (byte) d;
                    if (images[rxDrvno] != null) {
                        if (rxCount == images[rxDrvno].bytesPerTrack()) {
                            rxCount = 0;
                            rxState = RxState.IDLE;
                            try {
                                images[rxDrvno].write(rxTrack, rxOffset);
                                callback.onTrackWritten(rxDrvno, rxOffset / images[rxDrvno].bytesPerTrack());
                            } catch (IOException e) {
                                callback.onWriteError(rxDrvno, e.getMessage());
                            }
                        }
                    } else {
                        rxState = RxState.IDLE;
                    }
                }
            }
        }
    }

    private int slipDecode(byte b) {
        if (slipEscaping) {
            slipEscaping = false;
            if (b == ESC_END) {
                return END;
            } else if (b == ESC_ESC) {
                return ESC;
            } else {
                return RX_ERROR;
            }
        } else {
            if (b == END) {
                return RX_END;
            } else if (b == ESC) {
                slipEscaping = true;
                return RX_ESC;
            } else {
                return 0xff & b;
            }
        }
    }

    private byte[] slipEncode(byte[] data) {
        int count = 0;
        byte[] tmp = new byte[2 * data.length];
        for (byte b : data) {
            if (b == END) {
                tmp[count++] = ESC;
                tmp[count++] = ESC_END;
            } else if (b == ESC) {
                tmp[count++] = ESC;
                tmp[count++] = ESC_ESC;
            } else {
                tmp[count++] = b;
            }
        }
        return Arrays.copyOf(tmp, count);
    }

    private String idToString(byte[] id) {
        StringBuilder b = new StringBuilder();
        b.append("[");
        for (int i = 0; i < id.length; i++) {
            if (i > 0) {
                b.append(", ");
            }
            b.append(String.format("%02x", 0xff & id[i]));
        }
        b.append("]");
        return b.toString();
    }

    private int getRootblockByte(byte[] image, int offset, int shift) {
        return ((int) image[ROOTBLOCK_OFFSET+offset] & 0xff) << shift;
    }

    public void createEmptyADFImage(String dir, String name) throws Exception {
        File file = new File(dir, name);
        Log.d(TAG, "writing " + file);

        String imageName = name.substring(0, name.indexOf("."));
        byte[] image = new byte[FloppyImage.ADF_BYTES_PER_TRACK * FloppyImage.TRACKS_PER_DISK];
        int offset = 0;
        for (Object obj : emptyADFImage) {
            if (obj instanceof byte[]) {
                byte[] bs = (byte []) obj;
                for (int j = 0; j < bs.length; j++) {
                    image[offset++] = bs[j];
                }
            } else {
                if (offset == DISKNAME_LENGTH_OFFSET) {
                    image[offset++] = (byte) min(imageName.length(), 30);
                } else if (offset == DISKNAME_OFFSET) {
                    byte[] bs = imageName.getBytes();
                    for (int j = 0; j < 30; j++) {
                        if (j < bs.length) {
                            image[offset++] = bs[j];
                        } else {
                            image[offset++] = 0;
                        }
                    }
                } else {
                    for (int j = 0; j < (int) obj; j++) {
                        image[offset++] = 0;
                    }
                }
            }
        }

        image[ROOTBLOCK_CKSUM_OFFSET+0] = 0;
        image[ROOTBLOCK_CKSUM_OFFSET+1] = 0;
        image[ROOTBLOCK_CKSUM_OFFSET+2] = 0;
        image[ROOTBLOCK_CKSUM_OFFSET+3] = 0;
        int cksum = 0;
        for (int j = 0; j < 512; j+=4) {
            cksum += getRootblockByte(image, j, 24) | getRootblockByte(image, j+1, 16) | getRootblockByte(image, j+2, 8) | getRootblockByte(image, j+3, 0);
        }
        cksum = -cksum;
        image[ROOTBLOCK_CKSUM_OFFSET+0] = (byte) ((cksum >> 24) & 0xff);
        image[ROOTBLOCK_CKSUM_OFFSET+1] = (byte) ((cksum >> 16) & 0xff);
        image[ROOTBLOCK_CKSUM_OFFSET+2] = (byte) ((cksum >>  8) & 0xff);
        image[ROOTBLOCK_CKSUM_OFFSET+3] = (byte) ((cksum      ) & 0xff);

        file.createNewFile();
        BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(file));
        out.write(image);
        out.close();
    }

    public void createEmptyRawImage(String dir, String name) throws Exception {
        File file = new File(dir, name);
        Log.d(TAG, "writing " + file);

        byte[] image = new byte[FloppyImage.RAW_BYTES_PER_TRACK * FloppyImage.TRACKS_PER_DISK];
        for (int i = 0; i < image.length; i++) {
            image[i] = (byte) 0xaa;
        }
        file.createNewFile();
        BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(file));
        out.write(image);
        out.close();
    }
}
