#!/usr/bin/env python

from random import randint, random

def gen_rand_array(n):
    for i in xrange(n * n):
        print random() * randint(1, 100),
        if i % n == n - 1:
            print
