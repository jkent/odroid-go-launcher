#!/usr/bin/env python3
import os
import PIL.Image
import sys

directory = sys.argv[1]

image_filenames = []
for _, _, filenames in os.walk(directory):
    filenames = filter(lambda s: s.endswith('.png'), filenames)
    image_filenames.extend(filenames)
    break

image_filenames.sort()

bitmaps = []

for image_filename in image_filenames:
    image = PIL.Image.open(os.path.join(directory, image_filename))
    image = image.convert('1')
    d = {}
    d['filename'] = image_filename
    d['width'], d['height'] = image.size
    d['bytes'] = image.tobytes()
    bitmaps.append(d)

width = bitmaps[0]['width']
height = bitmaps[0]['height']
num_bytes = len(bitmaps[0]['bytes'])

output = """#include "tf.h"

static const unsigned char fontBits_icons_%dX%d[%d][%d] = {
""" % (width, height, len(bitmaps), num_bytes)
i = 32
for bitmap in bitmaps:
  output += "  {"
  for byte in bitmap['bytes']:
    output += "0x%02X," % (byte,)
  output += "}, // 0x%02X '%s'\n" % (i, chr(i)) 
  i += 1
output += """};

const struct tf_font font_icons_%dX%d = {(const unsigned char *)fontBits_icons_%dX%d, %d, %d, 32, %d, NULL};

""" % (width, height, width, height, width, height, i - 1)

i = 32
for bitmap in bitmaps:
  name, _ = os.path.splitext(os.path.basename(bitmap['filename']))
  output += "#define FONT_ICON_%s (0x%02X)\n" % (name.upper(), i)
  i += 1

f = open('icons_%dX%d.c' % (width, height), 'w')
f.write(output)
f.close()

