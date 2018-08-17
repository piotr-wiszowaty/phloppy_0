#!/usr/bin/env python

import argparse
import datetime
import mmap
import Queue
import socket
import sys
import threading
import time

END = "\xc0"
ESC = "\xdb"
ESC_END = "\xdc"
ESC_ESC = "\xdd"

OP_NOP      = "\x00"
OP_INSERT0  = "\x01"
OP_INSERT1  = "\x02"
OP_EJECT0   = "\x03"
OP_EJECT1   = "\x04"
OP_FILL0    = "\x05"
OP_FILL1    = "\x06"
OP_WPROT0   = "\x07"
OP_WPROT1   = "\x08"
OP_WUNPROT0 = "\x09"
OP_WUNPROT1 = "\x0a"
OP_INSERT2  = "\x11"
OP_INSERT3  = "\x12"
OP_EJECT2   = "\x13"
OP_EJECT3   = "\x14"
OP_FILL2    = "\x15"
OP_FILL3    = "\x16"
OP_WPROT2   = "\x17"
OP_WPROT3   = "\x18"
OP_WUNPROT2 = "\x19"
OP_WUNPROT3 = "\x1a"


class Emulator(threading.Thread):
    def __init__(self, address, port):
        threading.Thread.__init__(self)
        self.path = ["", "", "", ""]
        self.file = [None, None, None, None]
        self.image = [None, None, None, None]
        self.write_protection = [True, True, True, True]
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1048576)
        self.sock.connect((address, port))
        self.mq1 = Queue.Queue()
        self.mq2 = Queue.Queue()
        self.rx_state = "IDLE"
        self.escaping = False
        self.rx_thread = threading.Thread(target=self.rx_loop)
        self.rx_thread.start()

    def close(self):
        self.mq1.put(("EXIT", None))

    def insert(self, drvno, path):
        self.mq1.put(("INSERT", (drvno, path)))
        return self.mq2.get()

    def eject(self, drvno):
        self.mq1.put(("EJECT", drvno))
        return self.mq2.get()

    def write_protect(self, drvno, flag):
        self.mq1.put(("WPROT", (drvno, flag)))
        return self.mq2.get()

    def run(self):
        while True:
            try:
                if self.rx_state != "IDLE":
                    c, args = self.mq1.get_nowait()
                else:
                    c, args = self.mq1.get(True, 0.5)
            except Queue.Empty:
                c, args = "TIMEOUT", None

            if c == "EXIT":
                self.sock.shutdown(socket.SHUT_RDWR)
                self.sock.close()
                self.close_image(0)
                self.close_image(1)
                self.close_image(2)
                self.close_image(3)
                break
            elif c == "INSERT":
                drvno, path = args
                try:
                    self.close_image(drvno)
                    self.open_image(drvno, path)
                    data = self.file[drvno].read()
                    if drvno == 0:
                        self.send(END, OP_EJECT0, END, OP_FILL0, self.slip_encode(data), END, OP_INSERT0)
                    elif drvno == 1:
                        self.send(END, OP_EJECT1, END, OP_FILL1, self.slip_encode(data), END, OP_INSERT1)
                    elif drvno == 2:
                        self.send(END, OP_EJECT2, END, OP_FILL2, self.slip_encode(data), END, OP_INSERT2)
                    else:
                        self.send(END, OP_EJECT3, END, OP_FILL3, self.slip_encode(data), END, OP_INSERT3)
                    self.mq2.put("")
                except Exception, msg:
                    self.path[drvno] = ""
                    self.mq2.put(msg)
            elif c == "EJECT":
                self.close_image(drvno)
                self.send(END, [OP_EJECT0, OP_EJECT1, OP_EJECT2, OP_EJECT3][drvno])
                self.mq2.put("")
            elif c == "WPROT":
                drvno, flag = args
                self.write_protection[drvno] = flag
                if flag:
                    self.send(END, [OP_WPROT0, OP_WPROT1, OP_WPROT2, OP_WPROT3][drvno])
                else:
                    self.send(END, [OP_WUNPROT0, OP_WUNPROT1, OP_WUNPROT2, OP_WUNPROT3][drvno])
                self.mq2.put("")
            elif c == "TIMEOUT":
                if self.rx_state != "IDLE":
                    self.send(END, OP_NOP, "\x00" * (4096-2))
                else:
                    self.send(END, OP_NOP)
            elif c == "RX":
                self.process_rx(args)

    def open_image(self, drvno, path):
        self.path[drvno] = path
        self.file[drvno] = open(path, "r+b")
        self.image[drvno] = mmap.mmap(self.file[drvno].fileno(), 0)

    def close_image(self, drvno):
        if self.path[drvno]:
            self.path[drvno] = ""
            self.image[drvno].close()
            self.file[drvno].close()

    def send(self, *args):
        chunk_size = 64
        data = "".join(args)
        tail_length = chunk_size - (len(data) % chunk_size)
        if tail_length == 1:
            padding = "".join([END, OP_NOP, "\x00" * 63])
        elif tail_length:
            padding = "".join([END, OP_NOP, "\x00" * tail_length])
        else:
            padding = ""
        self.sock.sendall("".join([data, padding]))

    def rx_loop(self):
        while True:
            try:
                data = self.sock.recv(4096)
                self.mq1.put(("RX", data))
            except Exception:
                break

    def process_rx(self, data):
        #print "%d:%s" % (len(data), repr(data.replace("\x00", ".")))
        for c in data:
            d = self.slip_decode(c)
            if self.rx_state == "IDLE":
                if d == "end":
                    self.rx_state = "FLOPPY_NUM"
            elif self.rx_state == "FLOPPY_NUM":
                if d == "esc":
                    pass
                elif d == "end" or d == "error":
                    self.rx_state = "IDLE"
                    sys.stderr.write("[E0]"); sys.stderr.flush()
                else:
                    self.rx_state = "TT"
                    self.rx_drvno = ord(d)
                    if self.write_protection[self.rx_drvno]:
                        self.rx_state = "IDLE"
            elif self.rx_state == "TT":
                if d == "esc":
                    pass
                elif d == "end" or d == "error":
                    self.rx_state = "IDLE"
                    sys.stderr.write("[E1]"); sys.stderr.flush()
                else:
                    self.rx_state = "TRANSMIT"
                    self.rx_offset = ord(d) * 512*11
                    self.rx_end = self.rx_offset + 512*11
            elif self.rx_state == "TRANSMIT":
                if d == "esc":
                    pass
                elif d == "end" or d == "error":
                    self.rx_state = "IDLE"
                    sys.stderr.write("[E2]"); sys.stderr.flush()
                else:
                    self.image[self.rx_drvno][self.rx_offset] = d
                    self.rx_offset += 1
                    if self.rx_offset == self.rx_end:
                        self.rx_state = "IDLE"

    def slip_decode(self, c):
        if self.escaping:
            self.escaping = False
            if c == ESC_END:
                return END
            elif c == ESC_ESC:
                return ESC
            else:
                return "error"
        else:
            if c == END:
                return "end"
            elif c == ESC:
                self.escaping = True
                return "esc"
            else:
                return c

    def slip_encode(self, data):
        return data.replace(ESC, ESC+ESC_ESC).replace(END, ESC+ESC_END)

    def __str__(self):
        return "drive 0: %s [write protection = %s]\ndrive 1: %s [write protection = %s]\ndrive 2: %s [write protection = %s]\ndrive 3: %s [write protection = %s]" % (self.path[0], ["off", "on"][self.write_protection[0]], self.path[1], ["off", "on"][self.write_protection[1]], self.path[2], ["off", "on"][self.write_protection[2]], self.path[3], ["off", "on"][self.write_protection[3]])


parser = argparse.ArgumentParser()
parser.add_argument("-a", "--address", metavar="ADDRESS", default="192.168.4.1", help="drive IP address")
parser.add_argument("-p", "--port", metavar="PORT", default=4500, type=int, help="drive TCP port")
args = parser.parse_args()

emu = Emulator(args.address, args.port)
emu.start()

while True:
    try:
        line = raw_input("phloppy> ")
    except EOFError:
        print ""
        break
    line = line.strip()
    if line == "":
        continue
    tokens = line.strip().split()
    cmd = tokens[0]
    if cmd.startswith("q") or cmd.startswith("exit"):
        break
    elif "insert".startswith(cmd):
        if len(tokens) >= 3 and tokens[1] in ["0", "1", "2", "3"]:
            #t0 = datetime.datetime.now()
            errmsg = emu.insert(int(tokens[1]), " ".join(tokens[2:]))
            #t1 = datetime.datetime.now()
            #print t1 - t0
            if errmsg: print errmsg
        else:
            print "usage: insert 0|1|2|3 PATH"
    elif "eject".startswith(cmd):
        if len(tokens) == 2 and tokens[1] in ["0", "1", "2", "3"]:
            emu.eject(int(tokens[1]))
        else:
            print "usage: e[ject] 0|1|2|3"
    elif "status".startswith(cmd):
        print emu
    elif "protect".startswith(cmd):
        if len(tokens) == 2 and tokens[1] in ["0", "1", "2", "3"]:
            emu.write_protect(int(tokens[1]), True)
        else:
            print "usage: p[rotect] 0|1|2|3"
    elif "unprotect".startswith(cmd):
        if len(tokens) == 2 and tokens[1] in ["0", "1", "2", "3"]:
            emu.write_protect(int(tokens[1]), False)
        else:
            print "usage: u[nprotect] 0|1|2|3"
        pass
    elif "help".startswith(cmd):
        print "commands:\n"
        print "q[uit], exit           - exit program"
        print "i[nsert] 0|1|2|3 PATH  - insert floppy image"
        print "e[ject] 0|1|2|3        - eject floppy image"
        print "s[tatus]               - print current status"
        print "p[rotect] 0|1|2|3      - write protect floppy image"
        print "u[nprotect] 0|1|2|3    - write un-protect floppy image"
        print "h[elp]                 - print this information"
        print ""

emu.close()
