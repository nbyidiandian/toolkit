#!/usr/bin/env python

import sys
import os

class uniq:
    def __init__(self, max_buffer_size):
        self._max_buffer_size = max_buffer_size
        if max_buffer_size > 0:
            self._buffer = [None] * max_buffer_size
        else:
            self._buffer = []
        self._map = {}
        self._cursor = 0

    def _exec(self, k, v):
        print '%s\t%d' % (k, v)

    def update(self, l):
        if self._max_buffer_size == 0:
            try:
                self._map[l] += 1
            except:
                self._map[l] = 1
        else:
            self._updateWithBuffer(l)

    def _updateWithBuffer(self, l):
        try:
            self._map[l] += 1
        except:
            self._cursor %= self._max_buffer_size
            evict_item = self._buffer[self._cursor]
            if evict_item:
                self._exec(evict_item, self._map[evict_item])
                del self._map[evict_item]
            self._buffer[self._cursor] = l
            self._map[l] = 1
        self._cursor += 1

    def finish(self):
        for k, v in self._map.iteritems():
            self._exec(k, v)

def usage() :
    print '''Usage: %s [-k|--key=KEY] [-m|--max_buffer_size=NUM]
    As uniq(1) on linux, without sort.
    ''' % sys.argv[0]

if __name__ == '__main__':
    import sys
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'tk:m:h?', \
                ['key=', 'max-buffer-size=', 'help'])
    except getopt.GetoptError, err:
        print str(err)
        print 'use \"-h\" \"-?\" or \"--help\" get help information'
        sys.exit(1)

    key = -1
    max_buffer_size = 0
    unit_test = False
    for o, a in opts:
        if o in ['-k', '--key']:
            try:
                key = int(a)
            except:
                print 'invalid option %s %s' % (o, a)
                sys.exit(1)
        elif o in ['-m', '--max-buffer-size']:
            try:
                max_buffer_size = int(a)
            except:
                print 'invalid option %s %s' % (o, a)
                sys.exit(1)
        elif o in ['-t', '--test']:
            unit_test = True
        elif o in ['-h', '-?', '--help'] :
            usage()
            sys.exit(0)
        else :
            assert False, 'unhandled option'

    u = uniq(max_buffer_size)
    while True:
        l = sys.stdin.readline()
        if not l:
            break

        l = l.strip()
        u.update(l)

    u.finish()
