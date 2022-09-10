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

  const std::mutex m_dataMutex;

  // Most up to date data to come in over websockets
  // At some point this will become a struct i promise
  wpi::json m_data;
};



static constexpr uv::Timer::Time kReconnectRate{1000};
static constexpr uv::Timer::Time kWebsocketHandshakeTimeout{500};


void WebSocketClient::WsConnected(wpi::WebSocket& ws, uv::Tcp& tcp) {
  m_parallelConnect->Succeeded(tcp);

  std::string ip;
  unsigned int port;
  uv::AddrToName(tcp.GetPeer(), &ip, &port);

  fmt::print("CONNECTED to {} port {}\n", ip, port);

  isConnected = true;

  ws.text.connect([](std::string_view data, bool) {
    printf("Client got text: %s\n", std::string(data).c_str());
  });
  ws.binary.connect([](wpi::span<const uint8_t> data, bool) {
    printf("Client got binary\n");
  });
}

void WebSocketClient::TcpConnected(uv::Tcp& tcp) {
  // We get this printout twice
  printf("Got tcp!\n");

  tcp.SetNoDelay(true);
  // Start the WS client
  wpi::WebSocket::ClientOptions options;
  options.handshakeTimeout = kWebsocketHandshakeTimeout;
  auto m_id = "";
  auto ws =
      wpi::WebSocket::CreateClient(tcp, "/", "ws://127.0.0.1:9002",
                                   {{"frcvision", "13"}}, options);
  
  ws->open.connect([&tcp, ws = ws.get()](std::string_view) {
    printf("WS opened!\n");
    WsConnected(*ws, tcp);
  });

  ws->closed.connect([&](int, std::string_view){
    printf("WS closed!\n");
    isConnected = false;
    m_parallelConnect->Disconnected();
  });
}

WebSocketClient::WebSocketClient() {
  m_loopRunner.GetLoop()->error.connect(
      [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

  m_loopRunner.ExecAsync([&](uv::Loop& loop) {
    m_parallelConnect = wpi::ParallelTcpConnector::Create(
        loop, kReconnectRate, m_logger,
        [](uv::Tcp& tcp) { TcpConnected(tcp); });

    const std::pair<std::string, unsigned int> server = {"127.0.0.1", 9002};
    m_parallelConnect->SetServers(std::array{server});
  });

}

int main() {
  uv::Process::DisableStdioInheritance();

  auto client = WebSocketClient();

  std::this_thread::sleep_for(std::chrono::milliseconds(20000));
}