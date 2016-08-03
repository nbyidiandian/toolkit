#!/bin/env python

import sys

from uniq import uniq

u = uniq()

while True :
	l = sys.stdin.readline()
	if not l :
		break

	l = l.strip()

	k, v = l.split('\t')
	
	u.update(k, int(v))

u.finish()
