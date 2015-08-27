#!/usr/bin/env python
# Build multi-stage lookup tables for character widths and grapheme break properties
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

def grapheme_break(c):
    return GraphemeBreak.get(c, 'Other')

class LookupTable(object):
    def __init__(self, label, row_len, val_bits, display_val):
        self.label = label
        self.row_len = row_len
        self.val_len = 1 << val_bits
        self.val_shift = 8 / self.val_len
        self.display_val = display_val
        self.blocks = []

    def append(self, block):
        try:
            index = self.blocks.index(block)
        except ValueError:
            index = len(self.blocks)
            self.blocks.append(block)
        return index

    def display(self):
        print 'static const uint8_t %s[][%d] = {' % (self.label, len(self.blocks[0])/self.val_len)
        for block in self.blocks:
            self.display_block(block)
        print '};\n'

    def display_block(self, block):
        print '\t{'
        for i in range(0, len(block), self.row_len):
            self.display_row(block[i:i+self.row_len])
        print '\t},'

    def display_row(self, row):
        vals = []
        for i in range(0, len(row), self.val_len):
            vals.append(self.display_val(row[i:i+self.val_len], self.val_shift))
        print '\t\t' + ', '.join(vals) + ','

def display_index(val, val_shift):
    return '%d' % sum([v << (i * val_shift) for i,v in enumerate(val)])

def display_width(val, val_shift):
    return '0x%02X' % sum([v << (i * val_shift) for i,v in enumerate(val)])

def display_grapheme_break(val, val_shift):
    return ' | '.join(['(UTF8_GRAPHEME_BREAK_' + v.upper() + ' << ' + str(i * val_shift) + ')' for i,v in enumerate(val)])

def display_index_table(label, val):
    print 'static const uint8_t ' + label + '[] = {'
    for i in range(0, len(val), 0x10):
        print '\t' + ', '.join(map(str, val[i:i+0x10])) + ','
    print '};\n'

def table(label, f, display, row_len, count, block1_bits, block2_bits, val_bits):
    val_shift = 8 / (1 << val_bits)
    table1 = LookupTable(label + str(1), 16, 0, display_index)
    table2 = LookupTable(label + str(2), row_len, val_bits, display)
    block1_len = 1 << (block1_bits + block2_bits + val_bits);
    block2_len = 1 << (block2_bits + val_bits);
    indextable = []
    for i in range(0, count, block1_len):
        block1 = []
        for j in range(0, block1_len, block2_len):
            block2 = []
            for c in range(block2_len):
                block2.append(f(i + j + c))
            block1.append(table2.append(block2))
        indextable.append(table1.append(block1))

    table2.display()
    table1.display()
    display_index_table(label + str(0), indextable)
    print 'static const int %s0_shift = %d;' % (label, block1_bits + block2_bits + val_bits)
    print 'static const int %s1_shift = %d;' % (label, block2_bits + val_bits)
    print 'static const int %s2_shift = %d;' % (label, val_bits)
    print 'static const int %s1_mask = 0x%x;' % (label, (1 << block1_bits) - 1)
    print 'static const int %s2_mask = 0x%x;' % (label, (1 << block2_bits) - 1)
    print 'static const int %s3_mask = 0x%x;' % (label, (1 << val_bits) - 1)
    print 'static const int %s_val_shift = %d;' % (label, val_shift)
    print 'static const int %s_val_mask = 0x%x;' % (label, (1 << val_shift) - 1)

table('width', width, display_width, 32, 0x110000, 5, 3, 2)
print ''
table('grapheme_break', grapheme_break, display_grapheme_break, 2, 0x110000, 6, 3, 1)
