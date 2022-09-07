#include <cstdio>
#include <memory>
#include <vector>
#include <wpi/span.h>
#include "wpinet/uv/Loop.h"
#include "wpinet/uv/Tcp.h"
#include "wpinet/uv/Pipe.h"
#include "wpinet/uv/Timer.h"
#include "wpinet/WebSocket.h"  // NOLINT(build/include_order)
#include "WebSocketTest.h"
#include <wpi/StringExtras.h>
#include "wpinet/HttpParser.h"
#include <unistd.h>
#include <string_view>
#include <iostream>

using namespace wpi;

int main() {
  std::shared_ptr<uv::Loop> loop;
  std::shared_ptr<uv::Tcp> serverPipe;
  HttpParser resp{HttpParser::kResponse};
  std::shared_ptr<WebSocket> ws_server;

  loop = uv::Loop::Create();
  serverPipe = uv::Tcp::Create(loop);

  auto failTimer = uv::Timer::Create(loop);
  failTimer->timeout.connect([&] {
    loop->Stop();
    // FAIL() << "loop failed to terminate";
  });
  failTimer->Start(uv::Timer::Time{5000});
  failTimer->Unreference();

  serverPipe->Listen([&]() {
    auto conn = serverPipe->Accept();
    ws_server = WebSocket::CreateServer(*conn, "/test", "13");
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    ws_server->open.connect([&](std::string_view) {
      printf("Sending data!\n");
      ws_server->SendText({{data}}, [](auto bufs, uv::Error) {
        // This callback is called after the data's sent

        // ws->Terminate();
        // ASSERT_FALSE(bufs.empty());
        // ASSERT_EQ(bufs[0].base, reinterpret_cast<const char*>(data.data()));
      });
    });
  });

  loop->Run();

  // Cleanup
  loop->Walk([](uv::Handle& it) { it.Close(); });
}

// int main() {
//   wpi::EventLoopRunner runner;
//   std::shared_ptr<uv::Tcp> tcp;

//   runner.ExecAsync([&](auto& loop) {
//     loop.error.connect(
//         [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n",
//         err.str());
//         });

//     tcp = uv::Tcp::Create(loop);
//     const char* pipeName = "127.0.0.1";
//     const int port = 9002;

//     // Our callback should be called when the TCP connection opens
//     tcp->Connect(pipeName, port, [&] {
//       printf("Got TCP connection!\n");
//       auto ws = wpi::WebSocket::CreateClient(*tcp.get(), "/ws",
//         fmt::format("{}:{}", pipeName, port));

//       SendWsText(ws, {{"command", "SET_STATE"}, {"newState", "Foobar"}});
//     });
//   });

//   std::this_thread::sleep_for(std::chrono::milliseconds(10000000));
// }