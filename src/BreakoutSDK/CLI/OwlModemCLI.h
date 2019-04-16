/*
 * OwlModemCLI.h
 * Twilio Breakout SDK
 *
 * Copyright (c) 2018 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file OwlModemCLI.h - A CLI interface for human interaction with the OwlModem API
 */

#ifndef __OWL_MODEM_CLI_H__
#define __OWL_MODEM_CLI_H__

#include "../modem/enums.h"
#include "../modem/OwlModem.h"



#define MODEM_CLI_CMD_HISTORY 16
#define MODEM_CLI_CMD_LEN 256
#define MAX_CMD_PARAMS 32

#define CLI_PROMPT "OwlModem-CLI > "



class OwlModemCLICommand {
 public:
  str name;
  int argc;
  str argv[MAX_CMD_PARAMS];
  str args;

  int parse(str input) {
    str token = {0};
    args.s    = 0;
    args.len  = 0;
    name.s    = 0;
    name.len  = 0;
    argc      = 0;
    for (int i = 0; str_tok(input, " \t", &token); i++) {
      if (!i) {
        name = token;
      } else {
        if (i == 1) {
          args.s   = token.s;
          args.len = input.len - (args.s - input.s);
        }
        if (argc < MAX_CMD_PARAMS)
          argv[argc++] = token;
        else
          return 0;
      }
    }
    return 1;
  }
};

class OwlModemCLI;

class OwlModemCLIExecutor {
 public:
  str name           = {0};
  str helpParameters = {0};
  str help           = {0};
  int minParams      = 0;
  int maxParams      = 0;

  OwlModemCLIExecutor(char *c_name, char *c_help_params, char *c_help, int min, int max);

  OwlModemCLIExecutor(char *c_name, char *c_help);

  virtual ~OwlModemCLIExecutor();

  int checkParams(const OwlModemCLICommand &command);

  virtual void executor(OwlModemCLI &cli, OwlModemCLICommand &command);

 protected:
  static OwlModemCLI *savedCLI;
};



/**
 * CLI for the OwlModem
 */
class OwlModemCLI {
 public:
  OwlModemCLI(OwlModem *modem, USBSerial *debugPort);

  ~OwlModemCLI();


  /**
   * A simple CLI, to let you interactively test the API of OwlModem through the debug port. Call this function in the
   * main loop() to process user's input over USB serial and to execute commands towards the modem.
   * @param resume - 1 if to resume, 0 if this is a new start
   * @return 1 on success, 0 if exit was called
   */

  int handleUserInput(int resume);


  OwlModem *owlModem;
  USBSerial *debugPort;

  char cmdHistory[MODEM_CLI_CMD_HISTORY][MODEM_CLI_CMD_LEN + 1]; /**< Command history */
  int cmdHistoryLen[MODEM_CLI_CMD_HISTORY];                      /**< Length of the entries. Because, reasons. */
  int cmdHistoryHead;     /**< Points to the 'head' entry in the history, i.e. the last command entered.*/
  int cmdHistoryTail;     /**< Points to the 'tail' entry in the history, i.e. the oldest available command. The
                                 history is circular, so once it gets filled up, it should always point to current-1 */
  int cmdHistoryIterator; /**< Points to the currently selected command when using the arrow keys to scroll */


 private:
  OwlModemCLIExecutor **executors;
  OwlModemCLIExecutor *findExecutor(str name);
  void doHelp(str *prefix);
  void doHistory();
  void commandCompletion(str *command);

  char empty_spaces[MODEM_CLI_CMD_LEN + 1];
  char buf[MODEM_CLI_CMD_LEN + 1];
  str command            = {.s = buf, .len = 0};
  OwlModemCLICommand cmd = OwlModemCLICommand();
};

#endif
