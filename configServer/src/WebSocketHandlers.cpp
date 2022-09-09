// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "WebSocketHandlers.h"

#include <unistd.h>

#include <cstring>
#include <memory>

#include <fmt/format.h>
#include <wpi/SmallVector.h>
#include <wpi/StringExtras.h>
#include <wpinet/WebSocket.h>
#include <wpi/json.h>
#include <wpi/raw_ostream.h>
#include <wpinet/raw_uv_ostream.h>
#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Pipe.h>
#include <wpinet/uv/Process.h>

#include "SystemStatus.h"

#define EXEC_HOME  ""
#define APP_UID 1000
#define APP_GID 1000
#define NODE_HOME ""

namespace uv = wpi::uv;

extern bool romi;

struct WebSocketData {
  bool visionLogEnabled = false;
  bool romiLogEnabled = false;

  // Connection between the systemstatus status signal and our sendWSText function
  wpi::sig::ScopedConnection sysStatusConn;
};

static void SendWsText(wpi::WebSocket& ws, const wpi::json& j) {
  wpi::SmallVector<uv::Buffer, 4> toSend;
  wpi::raw_uv_ostream os{toSend, 4096};
  os << j;
  ws.SendText(toSend, [](wpi::span<uv::Buffer> bufs, uv::Error) {
    for (auto&& buf : bufs) buf.Deallocate();
  });
}

void InitWs(wpi::WebSocket& ws) {
  // set ws data, which can be later gotten with ws.GetData();
  auto data = std::make_shared<WebSocketData>();
  ws.SetData(data);

  // send initial system status and hook up system status updates
  auto sysStatus = SystemStatus::GetInstance();

  // Set up ws to send the system status when the systemstatus signal's emitted
  auto statusFunc = [&ws](const wpi::json& j) { SendWsText(ws, j); };
  statusFunc(sysStatus->GetStatusJson());
  data->sysStatusConn = sysStatus->status.connect_connection(statusFunc);
}

void ProcessWsText(wpi::WebSocket& ws, std::string_view msg) {
  fmt::print(stderr, "ws: '{}'\n", msg);

  // parse
  wpi::json j;
  try {
    j = wpi::json::parse(msg, nullptr, false);
  } catch (const wpi::json::parse_error& e) {
    fmt::print(stderr, "parse error at byte {}: {}\n", e.byte, e.what());
    return;
  }

  // top level must be an object
  if (!j.is_object()) {
    fmt::print(stderr, "{}", "not object\n");
    return;
  }

  SendWsText(ws, {{"type", "fileUploadComplete"}});

}

void ProcessWsBinary(wpi::WebSocket& ws, wpi::span<const uint8_t> msg) {
  auto d = ws.GetData<WebSocketData>();
  fmt::print(stderr,"ws: binary");
}
