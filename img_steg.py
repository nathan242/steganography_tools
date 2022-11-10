#!/usr/bin/python3

import sys
import getopt
from PIL import Image
import numpy

def help():
    print('Image steganography tool.')
    print('Usage: '+sys.argv[0]+ ' [OPTIONS] <input file>')
    print(' -s Show capacity of image in bytes.')
    print(' -o <file> Output file. If set will encode data.')
    print(' -d <data> Data to encode. If not set will use stdin.')

def encodeData(infile, outfilename, encode):
    arr = numpy.array(infile)
    encode += '\0'

    x = 0
    y = 0
    c = 0

    colours = len(arr[y][x])-1

    for char in encode:
        bit = 128
        while bit != 0:
            value = arr[y][x][c]
            if value % 2 != int(ord(char) & bit == bit):
                if value > 127:
                    value -= 1
                else:
                    value += 1

            arr[y][x][c] = value

            if bit == 1:
                bit = 0
            else:
                bit = int(bit/2)

            c += 1
            if c > colours:
                c = 0
                x += 1
                if x > infile.size[0]-1:
                    x = 0
                    y += 1
                    if y > infile.size[1]-1:
                        raise Exception('Out of space in image')

    out = Image.fromarray(arr)
    out.save(outfilename)

def decodeData(infile):
    arr = numpy.array(infile)

    chars = []
    char = 0
    bit = 128
    done = False

    for y in arr:
        if done is True:
            break

        for x in y:
            if done is True:
                break

            for c in x:
                if c % 2 != 0:
                    char += bit

                if bit == 1:
                    if char == 0:
                        done = True
                        break

                    chars.append(chr(char))
                    char = 0
                    bit = 128
                else:
                    bit = int(bit/2)

    return ''.join(chars)

def getMaxDataSize(infile):
    size = 0
    arr = numpy.array(infile)

    for y in arr:
        for x in y:
            size += len(x)

    return int(size/8)

encode = None
outfilename = None
getsize = False

try:
    optlist, args = getopt.getopt(sys.argv[1:], 'hso:d:')
except getopt.GetoptError as err:
    sys.stderr.write(str(err)+"\n")
    help()
    sys.exit(1)

for opt, arg in optlist:
    if opt == '-h':
        help()
        sys.exit(0)
    elif opt == '-s':
        getsize = True
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

if encode is not None and outfilename is None:
    sys.stderr.write("No output filename specified (-o)\n")
    sys.exit(1)

try:
    infile = Image.open(args[0])
except:
    sys.stderr.write("Cannot open input image\n")
    sys.exit(2)

if getsize is True:
    print(getMaxDataSize(infile))
elif outfilename is not None:
    if encode is None:
        encode = sys.stdin.read()

    try:
        encodeData(infile, outfilename, encode)
    except Exception as err:
        sys.stderr.write("Failed to encode data: "+str(err)+"\n")
        sys.exit(3)
else:
    try:
        print(decodeData(infile))
    except Exception as err:
        sys.stderr.write("Failed to encode data: "+str(err)+"\n")
        sys.exit(3)

