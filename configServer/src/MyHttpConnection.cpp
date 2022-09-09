// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "MyHttpConnection.h"

#include <unistd.h>
#include <uv.h>

#include <fmt/format.h>
#include <wpi/SmallVector.h>
#include <wpinet/UrlParser.h>
#include <wpi/fs.h>
#include <wpi/raw_ostream.h>
#include <wpinet/raw_uv_ostream.h>
#include <wpinet/uv/Request.h>

#include "WebSocketHandlers.h"

#define ZIPS_DIR "/home/pi/zips"

namespace uv = wpi::uv;

MyHttpConnection::MyHttpConnection(std::shared_ptr<wpi::uv::Stream> stream)
    : HttpServerConnection(stream), m_websocketHelper(m_request) {
  // Handle upgrade event
  m_websocketHelper.upgrade.connect([this] {
    // fmt::print(stderr, "{}", "got websocket upgrade\n");
    //  Disconnect HttpServerConnection header reader
    m_dataConn.disconnect();
    m_messageCompleteConn.disconnect();

    // Accepting the stream may destroy this (as it replaces the stream user
    // data), so grab a shared pointer first.
    auto self = shared_from_this();

    // Accept the upgrade
    m_ws = m_websocketHelper.Accept(m_stream, "frcvision");

    // Connect the websocket open event to our connected event.
    // Pass self to delay destruction until this callback happens
    m_ws->open.connect_extended([self, s = m_ws.get()](auto conn, auto) {
      fmt::print(stderr, "{}", "websocket connected\n");
      self->SendText("Hello, world!");
      InitWs(*s);
      conn.disconnect();  // one-shot
    });
    m_ws->text.connect(
        [s = m_ws.get()](auto msg, bool) { ProcessWsText(*s, msg); });
    m_ws->binary.connect(
        [s = m_ws.get()](auto msg, bool) { ProcessWsBinary(*s, msg); });
  });
}

void MyHttpConnection::SendText(std::string_view text) {
  if (m_ws) {
    wpi::SmallVector<uv::Buffer, 4> toSend;
    wpi::raw_uv_ostream os{toSend, 4096};
    os << text;
    m_ws->SendText(toSend, [](wpi::span<uv::Buffer> bufs, uv::Error) {
      for (auto&& buf : bufs)
        buf.Deallocate();
    });
  }
}
