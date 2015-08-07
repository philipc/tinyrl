#!/usr/bin/env python
# Build an ordered list of character widths and grapheme break properties
import re

CatCode = dict()
with open('UnicodeData.txt') as f:
    for line in f.readlines():
        tokens = line.split(';')
        if len(tokens) < 3:
            continue

        c = int(tokens[0], 16)
        if re.match('^<.*, First>$', tokens[1]):
            start = c
        elif re.match('^<.*, Last>$', tokens[1]):
            end = c
            for c in range(start, end + 1):
                CatCode[c] = tokens[2]
        else:
            CatCode[c] = tokens[2]

EastAsianWidth = dict()
with open('EastAsianWidth.txt') as f:
    for line in f.readlines():
        tokens = line.split('#')[0].split(';')
        if len(tokens) < 2:
            continue

        rangetokens = tokens[0].split('..')
        start = int(rangetokens[0], 16)
        if len(rangetokens) > 1:
            end = int(rangetokens[1], 16)
        else:
            end = start

        width = tokens[1].strip()
        for c in range(start, end + 1):
            if width=='W' or width=='F':
                EastAsianWidth[c] = 2
            elif width=='Na' or width=='H':
                EastAsianWidth[c] = 1

GraphemeBreak = dict()
with open('GraphemeBreakProperty.txt') as f:
    for line in f.readlines():
        tokens = line.split('#')[0].split(';')
        if len(tokens) < 2:
            continue

        rangetokens = tokens[0].split('..')
        start = int(rangetokens[0], 16)
        if len(rangetokens) > 1:
            end = int(rangetokens[1], 16)
        else:
            end = start

        b = tokens[1].strip()
        for c in range(start, end + 1):
            GraphemeBreak[c] = b

def width(c):
    if c == 0x00ad:
        return 1
    cat = CatCode.get(c, 'Cn')
    if cat == 'Mn' or cat == 'Me' or cat == 'Cc' or cat == 'Cf':
        return 0
    return EastAsianWidth.get(c, 1)

def display(first, last, w, b):
    if w != 0 or b != 'Other':
        print '\t{ 0x%04X, 0x%04X, %d, UTF8_GRAPHEME_BREAK_%s },' % (first, last, w, b.upper())

firstc = 0
lastw = 0
lastb = 'Other'
for c in range(0x110000):
    w = width(c)
    b = GraphemeBreak.get(c, 'Other')
    if w != lastw or b != lastb:
        display(firstc, c - 1, lastw, lastb)
        firstc = c
        lastw = w
        lastb = b
display(firstc, 0x10ffff, lastw, lastb)
