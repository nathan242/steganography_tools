#!/usr/bin/python3

import sys
import getopt
from PIL import Image
import numpy

def help():
    print('HELP')

def encodeData(infile, outfilename, encode):
    arr = numpy.array(infile)

    binary = []

    for char in encode:
        byte = format(ord(char), '08b')
        binary.append(byte)

    binary.append('00000000')

    x = 0
    y = 0
    c = 0
    for byte in binary:
        for bit in byte:
            value = arr[y][x][c]
            if value % 2 != int(bit) % 2:
                if value > 127:
                    value -= 1
                else:
                    value += 1

            arr[y][x][c] = value

            c += 1
            if c > 2:
                c = 0
                x += 1
                if x > infile.size[0]-1:
                    x = 0
                    y += 1
                    if y > infile.size[1]-1:
                        raise Exception("Out of space in image")

    out = Image.fromarray(arr)
    out.save(outfilename)

def decodeData(infile):
    arr = numpy.array(infile)

    data = ''
    binary = []
    raw = ''
    done = False

    for y in arr:
        if done is True:
            break

        for x in y:
            if done is True:
                break

            for c in x:
                raw += str(int(c % 2 != 0))
                if len(raw) == 8:
                    if raw == '00000000':
                        done = True
                        break

                    binary.append(raw)
                    raw = ''

    for byte in binary:
        data += chr(int(byte, 2))

    return data

encode = None
outfilename = None

try:
    optlist, args = getopt.getopt(sys.argv[1:], 'ho:d:')
except getopt.GetoptError as err:
    sys.stderr.write(str(err)+"\n")
    help()
    sys.exit(1)

for opt, arg in optlist:
    if opt == '-h':
        help()
        sys.exit(0)
    elif opt == '-o':
        outfilename = arg
    elif opt == '-d':
        encode = arg
    else:
        sys.stderr.write('Unknown option: '+str(opt)+"\n")
        sys.exit(1)

if len(args) != 1:
    sys.stderr.write("No input file\n")
    sys.exit(1)

if type(encode) != type(outfilename):
    sys.stderr.write("For encoding both an output file (-o) and data (-d) must be set\n")
    sys.exit(1)

try:
    infile = Image.open(args[0])
except:
    sys.stderr.write("Cannot open input image\n")
    sys.exit(2)

if encode is not None:
    try:
        encodeData(infile, outfilename, encode)
    except:
        sys.stderr.write("Failed to encode data\n")
        sys.exit(3)
else:
    try:
        print(decodeData(infile))
    except:
        sys.stderr.write("Failed to decode data\n")
        sys.exit(3)



