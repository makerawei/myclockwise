#pragma once
#include <Arduino.h>
#include <CWDateTime.h>

static const char *delim = "/"; // 分隔符

struct CommandController {
  static CommandController *getInstance() {
    static CommandController base;
    return &base;
  }
  
  void handleCommand(const char *cmd) {
    char *str = strdup(cmd);
    char *action = strtok(str, delim);
    if(action == NULL) {
      return;
    }
    if(strcmp(action, "alarm") == 0) {
      CWDateTime::getInstance()->handleAlarmCmd(strtok(NULL, ""));
    } else {
      Serial.printf("unknown action:%s\n", action);
    }

    free(str);
  }
};

