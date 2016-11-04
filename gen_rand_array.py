#!/usr/bin/env python

# Example: $ ./gen_rand_array.py 4

from random import randint, random
from sys import argv

def gen_rand_array(n):
    for i in xrange(n * n):
        print random() * randint(1, 100),
        if i % n == n - 1:
            print

if __name__ == '__main__':
    gen_rand_array(int(argv[1]))
