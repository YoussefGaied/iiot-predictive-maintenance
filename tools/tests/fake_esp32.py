import os
import time

master, slave = os.openpty()
print("Fake port:", os.ttyname(slave), flush=True)

os.write(master, b"ets Jun 8 2016 \xff\xfe garbage\r\n")
os.write(master, b"phase5_acs712_motor v0.1 (fake)\r\n")

i = 0
while True:
    mean = 2600 - 0.1 * i
    os.write(master, f"dir=0 duty=145 mean_mV={mean:.1f}\r\n".encode())
    i += 1
    time.sleep(0.5)

