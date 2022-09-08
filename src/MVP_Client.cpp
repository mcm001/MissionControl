#include <vector>

#include <wpi/span.h>

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

class WebSocketTest {
 public:
  static const char* pipeName;
  static const int port = 9002;

  static void SetUpTestCase();

  WebSocketTest() {
    loop = uv::Loop::Create();
    clientPipe = uv::Tcp::Create(loop);

    auto failTimer = uv::Timer::Create(loop);
    failTimer->timeout.connect([this] {
      loop->Stop();
      // FAIL() << "loop failed to terminate";
    });
    failTimer->Start(uv::Timer::Time{1000});
    failTimer->Unreference();

    resp.headersComplete.connect([this](bool) { printf("Client got headers\n"); headersDone = true; });

    clientPipe->Connect(pipeName, port, [this]() {
      clientPipe->StartRead();
      clientPipe->data.connect([this](uv::Buffer& buf, size_t size) {
        std::string_view data{buf.base, size};
        if (!headersDone) {
          data = resp.Execute(data);
          if (resp.HasError()) {
            printf("Client data error!");
            Finish();
          }
          // ASSERT_EQ(resp.GetError(), HPE_OK)
              // << http_errno_name(resp.GetError());
          if (data.empty()) {
            return;
          }
        }
        std::cout << "Data from server:\n";
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

  ~WebSocketTest() {
    Finish();
  }

  void Finish() {
    loop->Walk([](uv::Handle& it) { it.Close(); });
  }

  std::shared_ptr<uv::Loop> loop;
  std::shared_ptr<uv::Tcp> clientPipe;
  std::function<void()> setupWebSocket;
  std::function<void(std::string_view)> handleData;
  std::vector<uint8_t> wireData;
  std::shared_ptr<WebSocket> ws;
  HttpParser resp{HttpParser::kResponse};
  bool headersDone = false;
};


const char* WebSocketTest::pipeName = "127.0.0.1";

void WebSocketTest::SetUpTestCase() {
// #ifndef _WIN32
//   unlink(pipeName);
// #endif
}


}  // namespace wpi

int main() {
  using namespace wpi;
  WebSocketTest wsTest;

  int gotCallback = 0;
  // std::string str = "asdfasdfasdf";
  // std::vector<uint8_t> data = {str.begin(), str.end()};
  wsTest.setupWebSocket = [&] {
    wsTest.ws->open.connect([&](std::string_view) {
      printf("WS client connection opened!\n");
      wsTest.ws->text.connect([&](std::string_view text, bool) {
        printf("Text from server: %s\n", std::string(text).c_str());
      });
      wsTest.ws->binary.connect([&](wpi::span<const uint8_t> data, bool) {
        printf("Binary from server\n");
      });

      // wsTest.ws->SendText({{data}}, [&](auto bufs, uv::Error) {
      //   ++gotCallback;
      //   // wsTest.ws->Terminate();
      //   // ASSERT_FALSE(bufs.empty());
      //   // ASSERT_EQ(bufs[0].base, reinterpret_cast<const char*>(data.data()));
      // });
    });
  };

  wsTest.loop->Run();

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  // ASSERT_EQ(gotCallback, 1);
  return (gotCallback == 1);
}
// int main() {}