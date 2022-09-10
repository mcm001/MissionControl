#pragma once

#include <vector>

#include <wpi/span.h>
#include <wpi/Signal.h>
#include "wpinet/uv/Loop.h"
#include "wpinet/EventLoopRunner.h"
#include <wpi/Logger.h>
#include <wpinet/ParallelTcpConnector.h>
#include "wpinet/uv/Pipe.h"
#include "wpinet/uv/Tcp.h"
#include "wpinet/uv/Timer.h"
#include <wpinet/WebSocket.h>
#include <wpi/StringExtras.h>
#include <wpinet/uv/Process.h>
#include "wpinet/HttpParser.h"
#include <iostream>
#include <wpinet/uv/util.h>
#include <wpi/json.h>


using namespace wpi;
class WebSocketClient {

public:
WebSocketClient();
  void SetHost(std::string ip_addr, int port) {}

  static std::shared_ptr<WebSocketClient> GetInstance();

  wpi::json GetData();
  void SetData(const wpi::json&);

private:
  // Called when the parallel connector gets a TCP connection
  void TcpConnected(uv::Tcp& tcp);

  // Called on successful Websocket connection, possibly asyncronously?
  void WsConnected(wpi::WebSocket& ws, uv::Tcp& tcp);

  wpi::EventLoopRunner m_loopRunner;
  std::shared_ptr<wpi::ParallelTcpConnector> m_parallelConnect;
  std::shared_ptr<uv::Timer> m_sendPeriodicTimer;
  wpi::Logger m_logger;
  bool isConnected;

  std::mutex m_mutex;

  // Most up to date data to come in over websockets
  // At some point this will become a struct i promise
  wpi::json m_data;
};

