#include "WebSocketClient.h"

static constexpr uv::Timer::Time kReconnectRate{1000};
static constexpr uv::Timer::Time kWebsocketHandshakeTimeout{500};

void WebSocketClient::WsConnected(wpi::WebSocket& ws, uv::Tcp& tcp) {
  m_parallelConnect->Succeeded(tcp);

  std::string ip;
  unsigned int port;
  uv::AddrToName(tcp.GetPeer(), &ip, &port);

  fmt::print("CONNECTED to {} port {}\n", ip, port);

  isConnected = true;

  ws.text.connect([this](std::string_view data, bool) {
    printf("Client got text: %s\n", std::string(data).c_str());

    wpi::json j;
    try {
      j = wpi::json::parse(data, nullptr, false);
    } catch (const wpi::json::parse_error& e) {
      fmt::print(stderr, "parse error at byte {}: {}\n", e.byte, e.what());
      return;
    }

    // top level must be an object
    if (!j.is_object()) {
      fmt::print(stderr, "{}", "not object\n");
      return;
    }

    SetData(data);
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
  auto ws = wpi::WebSocket::CreateClient(tcp, "/", "ws://127.0.0.1:9002",
                                         {{"frcvision", "13"}}, options);

  ws->open.connect([&tcp, ws = ws.get(), this](std::string_view) {
    printf("WS opened!\n");
    WsConnected(*ws, tcp);
  });

  ws->closed.connect([&](int, std::string_view) {
    printf("WS closed!\n");
    isConnected = false;
    m_parallelConnect->Disconnected();
  });
}

WebSocketClient::WebSocketClient() : m_data(wpi::json::object()) {
  // Add hook to rpint on errors
  m_loopRunner.GetLoop()->error.connect(
      [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

  m_loopRunner.ExecAsync([&, this](uv::Loop& loop) {
    // This will auto-reconnect to the WS server, the lambda is a callback when
    // the connection happens
    m_parallelConnect = wpi::ParallelTcpConnector::Create(
        loop, kReconnectRate, m_logger,
        [this](uv::Tcp& tcp) { TcpConnected(tcp); });

    // TODO refactor this out into class members
    const std::pair<std::string, unsigned int> server = {"127.0.0.1", 9002};
    m_parallelConnect->SetServers(std::array{server});
  });
}

std::shared_ptr<WebSocketClient> WebSocketClient::GetInstance() {
  static auto instance = std::make_shared<WebSocketClient>();
  return instance;
}

wpi::json WebSocketClient::GetData() {
  wpi::json ret;
  {
    std::scoped_lock lk(m_mutex);
    ret = m_data;
  }
  return ret;
}

void WebSocketClient::SetData(const wpi::json& newData) {
  std::scoped_lock lk(m_mutex);
  m_data = newData;
}
