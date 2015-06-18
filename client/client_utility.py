import sys
import os
from random import randint
import time
import datetime


if not len(sys.argv) == 3:
    print("Must be two parameters: clients per second and request length (text length)")
    exit()


while (True):
    dt = datetime.datetime.now()
    curr_time = '%s:%s:%s' % (dt.hour, dt.minute, dt.second)
    print("Generated at %s" % curr_time)
    for i in range(0, int(sys.argv[1])):
        os.system("./client %s" % sys.argv[2])
    time.sleep(1)
