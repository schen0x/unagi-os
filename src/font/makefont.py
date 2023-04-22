#!/bin/python3

result = b'char hankaku[4096] = { '
with open('./hankaku.txt', 'rb') as fi:
    lines = fi.readlines()
    lines = lines[2:]
    for l in lines:
        l = l.strip();
        l = str(l, 'ascii')
        if len(l) < 1:
            continue
        if 'char' in l.lower():
            result = result[:-1] # remove the trailing space
            result += b'\n    '
            continue
        row_in_bitmap = b''
        for c in l:
            if c == ".":
                row_in_bitmap += b'0'
                continue
            if c == "*":
                row_in_bitmap += b'1'
                continue
        row_in_bitmap = hex(int(row_in_bitmap, 2)).encode('ascii')
        result += row_in_bitmap
        result += b', '

result = result[:-2]
result += b'\n};'

with open("./hankaku.c", "wb+") as fo:
    fo.write(result)

