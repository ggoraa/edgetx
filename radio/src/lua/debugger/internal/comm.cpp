/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <cli.h>
#include <eldp.pb.h>
#include <pb_common.h>
#include <pb_decode.h>

#include <string>

#include "../eldb.h"
#include "encode_decode.h"
#include "messages.h"
#include "session.h"

char eldbScriptToRun[128] = "";

void eldbReceive(uint8_t *rxBuf, size_t rxBufLen, size_t dataLen)
{
  if (dataLen != 0) {  // there actually IS some valuable data
    uint8_t txBuf[128] = {};
    size_t txLen = 0;
    edgetx_eldp_Request request = edgetx_eldp_Request_init_zero;
    char targetName[32] = "";

    pb_istream_t stream = pb_istream_from_buffer(rxBuf, dataLen);

    request.startDebug.targetName.funcs.decode = &decodeString;
    request.startDebug.targetName.arg = targetName;

    bool result = pb_decode(&stream, edgetx_eldp_Request_fields, &request);

    if (result) {
      if (request.has_startDebug && !eldbIsInSession()) {
        edgetx_eldp_Error_Type err;
        auto result = eldbStartSession(targetName, &err);
        if (result) {
          txLen = eldbMakeSystemInfoMessage(txBuf, sizeof(txBuf));
        } else {
          txLen = eldbMakeErrorMessage(txBuf, sizeof(txBuf), err, nullptr);
        }
      } else if (request.has_startDebug && eldbIsInSession()) {
        txLen = eldbMakeErrorMessage(txBuf, sizeof(txBuf),
                                     edgetx_eldp_Error_Type_ALREADY_STARTED,
                                     nullptr);
      } else if (!request.has_startDebug && eldbIsInSession()) {
        edgetx_eldp_Error_Type err;
        std::string msg;
        auto result = eldbForwardToRunningSession(&request, &err, &msg);
        if (!result) {
          txLen = eldbMakeErrorMessage(txBuf, sizeof(txBuf), err, msg.c_str());
        }
      } else {
        txLen = eldbMakeErrorMessage(txBuf, sizeof(txBuf),
                                     edgetx_eldp_Error_Type_NOT_STARTED_YET,
                                     nullptr);
      }
    } else {
      txLen = eldbMakeErrorMessage(txBuf, sizeof(txBuf),
                                   edgetx_eldp_Error_Type_BAD_MESSAGE,
                                   PB_GET_ERROR(&stream));
    }

    if (txLen > 0) cliELDPSend(txBuf, txLen);

    // TODO: Do proper error handling
  }
}