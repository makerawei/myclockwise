# -*- coding: utf-8 -*-
import pytz
from datetime import datetime

TzCodeTmpl = """#pragma once
namespace TzOffsetHelper {

  struct TzOffsetType {
    const char *timeZone;
    int offset;
  };

  struct TzOffsetType tzOffsets[] = {
%s
  };

  int offset(const char *timeZone) {
    for (int i = 0; i < sizeof(tzOffsets) / sizeof(TzOffsetType); i++) {
      if(strcmp(tzOffsets[i].timeZone, timeZone) == 0) {
        return tzOffsets[i].offset;
      }
    }
    return 0;
  }
}

"""

now = datetime.now()
timezones = []
for timezone_name in pytz.all_timezones:
    timezones.append((timezone_name, int(pytz.timezone(timezone_name).utcoffset(now).total_seconds())))
timezones.sort()

tz_code = ""
for name, offset in timezones:
    tz_code += '    {"%s", %d},\n' % (name, offset)

with open("TzOffsetHelper.h", "w") as fp:
    fp.write(TzCodeTmpl % tz_code[:-2])