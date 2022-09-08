#include <vector>

#include <wpi/span.h>

#include "gtest/gtest.h"
#include "wpinet/uv/Loop.h"
#include "wpinet/uv/Pipe.h"
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

  static void SetUpTestCase();

  WebSocketTest() {
    loop = uv::Loop::Create();
    clientPipe = uv::Pipe::Create(loop);
    serverPipe = uv::Pipe::Create(loop);

    serverPipe->Bind(pipeName);

    auto failTimer = uv::Timer::Create(loop);
    failTimer->timeout.connect([this] {
      loop->Stop();
      FAIL() << "loop failed to terminate";
    });
    failTimer->Start(uv::Timer::Time{1000});
    failTimer->Unreference();

    resp.headersComplete.connect([this](bool) { headersDone = true; });

    serverPipe->Listen([this]() {
      auto conn = serverPipe->Accept();
      ws = WebSocket::CreateServer(*conn, "foo", "13");
      if (setupWebSocket) {
        setupWebSocket();
      }
    });
    clientPipe->Connect(pipeName, [this]() {
      clientPipe->StartRead();
      clientPipe->data.connect([this](uv::Buffer& buf, size_t size) {
        std::string_view data{buf.base, size};
        if (!headersDone) {
          data = resp.Execute(data);
          if (resp.HasError()) {
            Finish();
          }
          ASSERT_EQ(resp.GetError(), HPE_OK)
              << http_errno_name(resp.GetError());
          if (data.empty()) {
            return;
          }
        }
        for (auto i : data) {
          std::cout << data;
        }
        std::cout << std::endl;
        wireData.insert(wireData.end(), data.begin(), data.end());
        if (handleData) {
          handleData(data);
        }
      });
      clientPipe->end.connect([this]() { Finish(); });
    });
  }

  ~WebSocketTest() override {
    Finish();
  }

  void Finish() {
    loop->Walk([](uv::Handle& it) { it.Close(); });
  }

  std::shared_ptr<uv::Loop> loop;
  std::shared_ptr<uv::Pipe> clientPipe;
  std::shared_ptr<uv::Pipe> serverPipe;
  std::function<void()> setupWebSocket;
  std::function<void(std::string_view)> handleData;
  std::vector<uint8_t> wireData;
  std::shared_ptr<WebSocket> ws;
  HttpParser resp{HttpParser::kResponse};
  bool headersDone = false;
};


#ifdef _WIN32
const char* WebSocketTest::pipeName = "\\\\.\\pipe\\websocket-unit-test";
#else
const char* WebSocketTest::pipeName = "/tmp/websocket-unit-test";
#endif

void WebSocketTest::SetUpTestCase() {
#ifndef _WIN32
  unlink(pipeName);
#endif
}

TEST_F(WebSocketTest, SendText) {
  int gotCallback = 0;
  std::string str = "owo?";
  std::vector<uint8_t> data = {str.begin(), str.end()};
  setupWebSocket = [&] {
    ws->open.connect([&](std::string_view) {
      ws->SendText({{data}}, [&](auto bufs, uv::Error) {
        ++gotCallback;
        ws->Terminate();
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