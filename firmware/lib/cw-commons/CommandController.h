#pragma once
#include <Arduino.h>
#include <CWDateTime.h>

static const char *delim = "/"; // 分隔符

struct CommandController {
  static CommandController *getInstance() {
    static CommandController base;
    return &base;
  }

  void onAlarmCmd(char *line) {
    Serial.printf("on larm cmd line=%s\n", line);
    char *cmdStr = strtok(line, delim);
    if(cmdStr == NULL) {
      return;
    }
    int cmd = String(cmdStr).toInt();    
    char *args = strtok(NULL, delim);
    switch(cmd) {
      case 1:
        if(CWDateTime::getInstance()->setAlarm(args)) {
          Serial.println("clock alarm set success");
        }
        break;
      default:
        break;
    }
    Serial.printf("on alarm cmd:%d, args:%s\n", cmd, args ? args : "");
  }
  
  void handleCommand(const char *cmd) {
    char *str = strdup(cmd);
    char *action = strtok(str, delim);
    if(action == NULL) {
      return;
    }
    if(strcmp(action, "alarm") == 0) {
      onAlarmCmd(strtok(NULL, ""));
    } else {
      Serial.printf("unknown action:%s\n", action);
    }

    free(str);
  }
};


