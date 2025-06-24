#ifndef __MAIN_H__
#define __MAIN_H__

typedef enum {
  CLOCK,
  RECORDING,
  WAITING,
  TOAST
} ClockState;

#define debugSerial Serial  // 定义调试打印的串口，不需要串口打印则注释这一行
#ifdef debugSerial
#define debugPrint(...) \
  { debugSerial.print(__VA_ARGS__); }
#define debugPrintln(...) \
  { debugSerial.println(__VA_ARGS__); }
#define debugPrintf(fmt, ...) \
  { debugSerial.printf(fmt, __VA_ARGS__); }
#else
#define debugPrint(...) \
  {}
#define debugPrintln(...) \
  {}
#define debugPrintf(fmt, ...) \
  {}
#endif

#endif

