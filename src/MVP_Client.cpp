#include <vector>

#include <wpi/span.h>
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

using namespace wpi;

static constexpr uv::Timer::Time kReconnectRate{1000};
static constexpr uv::Timer::Time kWebsocketHandshakeTimeout{500};

wpi::EventLoopRunner m_loopRunner;
std::shared_ptr<wpi::ParallelTcpConnector> m_parallelConnect;
std::shared_ptr<uv::Timer> m_sendPeriodicTimer;
wpi::Logger m_logger;
bool isConnected;

void WsConnected(wpi::WebSocket& ws, uv::Tcp& tcp) {
  m_parallelConnect->Succeeded(tcp);

  std::string ip;
  unsigned int port;
  uv::AddrToName(tcp.GetPeer(), &ip, &port);

  fmt::format("CONNECTED NT4 to {} port {}\n", ip, port);

  isConnected = true;

  ws.text.connect([](std::string_view data, bool) {
    printf("Client got text\n");
  });
  ws.binary.connect([](wpi::span<const uint8_t> data, bool) {
    printf("Client got binary\n");
  });
}

void TcpConnected(uv::Tcp& tcp) {
  // We get this printout twice
  printf("Got tcp!\n");

  tcp.SetNoDelay(true);
  // Start the WS client
  wpi::WebSocket::ClientOptions options;
  options.handshakeTimeout = kWebsocketHandshakeTimeout;
  auto m_id = "";
  auto ws =
      wpi::WebSocket::CreateClient(tcp, "", "ws://127.0.0.1:9002",
                                   {{"frcvision"}}, options);
  
  // Open never seems to get called
  ws->open.connect([&tcp, ws = ws.get()](std::string_view) {
    printf("WS opened!\n");
    WsConnected(*ws, tcp);
  });

  // Closed does get called twice, after the handshake timeout we set above has passed?
  ws->closed.connect([](int, std::string_view){
    printf("WS closed!\n");
    isConnected = false;
  });
}

int main() {
  uv::Process::DisableStdioInheritance();

  // No errors printed, good idea to have tho
  m_loopRunner.GetLoop()->error.connect(
      [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

  m_loopRunner.ExecAsync([&](uv::Loop& loop) {
    m_parallelConnect = wpi::ParallelTcpConnector::Create(
        loop, kReconnectRate, m_logger,
        [](uv::Tcp& tcp) { TcpConnected(tcp); });

    const std::pair<std::string, unsigned int> server = {"127.0.0.1", 9002};
    m_parallelConnect->SetServers(std::array{server});
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}