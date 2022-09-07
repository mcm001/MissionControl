#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Process.h>
#include <wpinet/uv/Tcp.h>
#include <wpinet/uv/Timer.h>
#include <wpinet/WebSocket.h>
#include <wpinet/uv/Udp.h>
#include <wpinet/EventLoopRunner.h>
#include <wpi/Logger.h>
#include <wpinet/ParallelTcpConnector.h>

namespace uv = wpi::uv;

void Finish(uv::Loop& loop) {
  loop.Walk([](uv::Handle& it) { it.Close(); });
}

int main() {
  wpi::EventLoopRunner runner;

  runner.ExecAsync([&](auto& loop) {
    loop.error.connect(
        [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

    auto tcp = uv::Tcp::Create(loop);
    const char* pipeName = "127.0.0.1";
    const int port = 9002;

    // Our callback should be called when the TCP connection opens
    tcp->Connect(pipeName, port, [&] {
      printf("Got TCP connection!\n");

      if (!tcp.get()) {
        printf("TCP is apparently nullptr?");
      }
      auto ws = wpi::WebSocket::CreateClient(*tcp.get(), "/wpilibws", 
        fmt::format("{}:{}", pipeName, port));
      ws->closed.connect([&](uint16_t code, std::string_view reason) {
        Finish(loop);
        if (code != 1005 && code != 1006) {
          // FAIL() << "Code: " << code << " Reason: " << reason;
        }
      });
      ws->open.connect([&](std::string_view protocol) {
        Finish(loop);
      });
    });
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
