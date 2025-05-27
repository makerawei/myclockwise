# -*- coding: utf-8 -*-
from conv_to_c_array import image_to_rgb565_array


text = ""
for i in range(10):
    w, h, arr = image_to_rgb565_array(f"num{i}.png")
    code = ""
    for j in range(h):
        line = arr[j * w : (j + 1) * w]
        code += (', '.join(line) + ",\n    ")
    text += "  {\r\n    %s\r\n  },\r\n\r\n" % code[:-6]

print("const uint16_t numbers[64][] = {")
print(text[:-5])
print("};")