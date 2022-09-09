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

namespace uv = wpi::uv;

extern bool romi;

static void SendWsText(wpi::WebSocket& ws, const wpi::json& j) {
  wpi::SmallVector<uv::Buffer, 4> toSend;
  wpi::raw_uv_ostream os{toSend, 4096};
  os << j;
  ws.SendText(toSend, [](wpi::span<uv::Buffer> bufs, uv::Error) {
    for (auto&& buf : bufs) buf.Deallocate();
  });
}

void ProcessWsText(wpi::WebSocket& ws, std::string_view msg) {
  fmt::print(stderr, "ws text: '{}'\n", msg);

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
  fmt::print(stderr,"ws binary: TODO");
}
