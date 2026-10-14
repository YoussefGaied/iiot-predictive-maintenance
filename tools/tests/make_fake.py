import random

random.seed(1)  # same "random" numbers every run

with open("fake_test.txt", "w") as f:
    f.write("phase5_acs712_motor v0.1 (fake test banner)\n")
    # Group 1: drifts down exactly 0.1 mV per line, plus noise
    for i in range(60):
        mean = 2600 - 0.1 * i + random.gauss(0, 0.3)
        f.write(f"dir=0 duty=145 mean_mV={mean:.1f} delta_mV=0.0\n")
    # Group 2: flat at 2650, plus noise
    for i in range(60):
        mean = 2650 + random.gauss(0, 0.3)
        f.write(f"dir=0 duty=255 mean_mV={mean:.1f} delta_mV=50.0\n")
    f.write("auto-stop\n")
