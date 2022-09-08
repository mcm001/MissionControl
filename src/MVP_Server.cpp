#include <vector>

#include <wpi/span.h>

#include "gtest/gtest.h"
#include "wpinet/uv/Loop.h"
#include "wpinet/uv/Pipe.h"
#include "wpinet/uv/Tcp.h"
#include "wpinet/uv/Timer.h"

#include <wpinet/WebSocket.h>  // NOLINT(build/include_order)
#include <wpi/StringExtras.h>
#include "wpinet/HttpParser.h"

#include <iostream>


namespace wpi {

namespace uv = wpi::uv;

class WebSocketTest : public ::testing::Test {
 public:
  static const char* pipeName;
  static const int port = 9002;

  static void SetUpTestCase();

  WebSocketTest() {
    loop = uv::Loop::Create();
    serverPipe = uv::Tcp::Create(loop);

    serverPipe->Bind(pipeName, 9002);

    auto failTimer = uv::Timer::Create(loop);
    failTimer->timeout.connect([this] {
      loop->Stop();
      FAIL() << "loop failed to terminate";
    });
    failTimer->Start(uv::Timer::Time{20000});
    failTimer->Unreference();

    resp.headersComplete.connect([this](bool) { headersDone = true; });

    serverPipe->Listen([this]() {
      auto conn = serverPipe->Accept();
      ws = WebSocket::CreateServer(*conn, "foo", "13");
      if (setupWebSocket) {
        setupWebSocket();
      }
    });
  }

  ~WebSocketTest() override {
    Finish();
  }

  void Finish() {
    loop->Walk([](uv::Handle& it) { it.Close(); });
  }

  std::shared_ptr<uv::Loop> loop;
  std::shared_ptr<uv::Tcp> clientPipe;
  std::shared_ptr<uv::Tcp> serverPipe;
  std::function<void()> setupWebSocket;
  std::function<void(std::string_view)> handleData;
  std::vector<uint8_t> wireData;
  std::shared_ptr<WebSocket> ws;
  HttpParser resp{HttpParser::kResponse};
  bool headersDone = false;
};


// #ifdef _WIN32
// const char* WebSocketTest::pipeName = "\\\\.\\pipe\\websocket-unit-test";
// #else
// const char* WebSocketTest::pipeName = "/tmp/websocket-unit-test";
// #endif
const char* WebSocketTest::pipeName = "127.0.0.1";

void WebSocketTest::SetUpTestCase() {
#ifndef _WIN32
  unlink(pipeName);
#endif
}

TEST_F(WebSocketTest, SendText) {
  int gotCallback = 0;
  std::string str = "asdf";
  std::vector<uint8_t> data = {str.begin(), str.end()};
  setupWebSocket = [&] {
    ws->open.connect([&](std::string_view) {
      printf("WS Server got client!\n");
      ws->SendText({{data}}, [&](auto bufs, uv::Error) {
        printf("WS Server sent text!\n");
        ++gotCallback;
        // ws->Terminate();
        ASSERT_FALSE(bufs.empty());
        ASSERT_EQ(bufs[0].base, reinterpret_cast<const char*>(data.data()));
      });
    });
  };

  loop->Run();

  ASSERT_EQ(gotCallback, 1);
}

}  // namespace wpi

// int main() {}