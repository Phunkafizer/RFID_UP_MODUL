from serial import *
import threading

SER_DEV = 'COM7'

STX = chr(0x02)
ETX = chr(0x03)
ESC = chr(0x1b)

CMD_SET_ADDRESS = 0x03
CMD_SET_PULSE_LEN = 0x04
CMD_OPEN_RELAY = 0x10
CMD_TAG_REQUEST = 0x20
CMD_READ_TAG = 0x30
CMD_READ_TAG_REP = 0x31
CMD_WRITE_TAG = 0x40
CMD_ADD_TAG = 0x48
CMD_IO_EVENT = 0x50

TAGTYPE_EM4100 = 0x01
TAGTYPE_FDXB = 0x02
TAGTYPE_NONE = 0XFF

uart = Serial(SER_DEV, 115200, timeout = 0.05, bytesize=EIGHTBITS, parity=PARITY_NONE)

def CalcCrc(l2data):
    crc = 0xFFFF
    for d in l2data:
        d = (d ^ crc) & 0xFF
        d = (d ^ (d << 4)) & 0xFF
        crc = ((d << 8) | (crc >> 8)) ^ (d >> 4) ^ (d << 3)
    return crc

def ParseFrame(frame):
    print frame
    dstadr = frame[0] | frame[1] << 8
    srcadr = frame[2] | frame[3] << 8
    frameId = frame[4]
    ack = frame[5]
    crc = frame[-2] | frame[-1] << 8

    if CalcCrc(frame) == 0:
        #print hex(srcadr) + "->" + hex(dstadr) + " (" + hex(frameId) + ") " + hex(crc)
        if len(frame) > 8:
            cmd = frame[6]
            data = frame[7:-2]

            if cmd == CMD_TAG_REQUEST:
                print "TAG REQUEST: ", data
            elif cmd == CMD_IO_EVENT:
                print "IO EVENT"
            elif cmd == CMD_READ_TAG_REP:
                print "read: ", data
            else:
                print "Unknown cmd", hex(cmd)
        else:
            print "empty frame"
        
    else:
        print "CRC failure!"

class RxThread(threading.Thread):
    def __init__(self):
        global uart
        self.__terminated__ = False
        threading.Thread.__init__(self)

    def run(self):
        rxdata = []
        inesc = False
        while not self.__terminated__:
            rx = uart.read(255)
            for c in rx:
                if inesc:
                    if c == STX:
                        rxdata = []
                    elif c == ETX:
                        ParseFrame(rxdata)
                    elif c == ESC:
                        rxdata.append(ord(c))
                    inesc = False
                else:
                    if c == ESC:
                        inesc = True
                    else:
                        rxdata.append(ord(c))

    def stop(self):
        self.__terminated__ = True
                
lasttxid = 0
def BuildL1Frame(dst, cmd, data, ack):
    global lasttxid
    lasttxid += 1
    buf = [dst & 0xFF, dst >> 8]
    buf += [0, 0]
    buf += [lasttxid]
    if ack:
        buf += [1]
    else:
        buf += [0]
    buf += [cmd]
    buf += data
    crc = CalcCrc(buf)
    buf += [crc & 0xFF, crc >> 8]

    ic = 0
    while ic < len(buf):
        if buf[ic] == ord(ESC):
            buf.insert(ic, ord(ESC))
            ic += 1
        ic += 1

    buf = [ord(ESC), ord(STX)] + buf + [ord(ESC), ord(ETX)]
    print "Buf:", buf
    uart.write(buf)

rxthread = RxThread()
rxthread.start()

while True:
    try:
        time.sleep(0.5)
    except (KeyboardInterrupt, SystemExit):
        break
    except:
        raise

rxthread.stop()
print "bye!"
