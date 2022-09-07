#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Process.h>
#include <wpinet/uv/Tcp.h>
#include <wpinet/uv/Timer.h>
#include <wpinet/WebSocket.h>
#include <wpinet/uv/Udp.h>
#include <wpinet/EventLoopRunner.h>
#include <wpi/Logger.h>
#include <wpi/json.h>
#include <wpinet/ParallelTcpConnector.h>
#include <wpinet/raw_uv_ostream.h>
#include <iostream>

namespace uv = wpi::uv;

void Finish(uv::Loop& loop) {
  loop.Walk([](uv::Handle& it) { it.Close(); });
}

static void SendWsText(std::shared_ptr<wpi::WebSocket> ws, const wpi::json& j) {
  wpi::SmallVector<uv::Buffer, 4> toSend;
  wpi::raw_uv_ostream os{toSend, 4096};
  os << j;
  printf("sending text\n");
  ws->SendText(toSend, [](wpi::span<uv::Buffer> bufs, uv::Error) {
    for (auto&& buf : bufs)
      buf.Deallocate();
  });
}

int main() {
  printf("Hello!\n");
  wpi::EventLoopRunner runner;
  std::shared_ptr<uv::Tcp> tcp;

  runner.ExecAsync([&](auto& loop) {
    loop.error.connect(
        [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

    tcp = uv::Tcp::Create(loop);
    const char* pipeName = "127.0.0.1";
    const int port = 9002;

    // Our callback should be called when the TCP connection opens
    tcp->Connect(pipeName, port, [&] {
      printf("Got TCP connection!\n");
      auto ws = wpi::WebSocket::CreateClient(
          *tcp.get(), "", fmt::format("{}:{}", pipeName, port));

      ws->open.connect([&tcp, &ws](std::string_view) {
        printf("Opened!\n");
        SendWsText(ws, {{"command", "SET_STATE"}, {"newState", "Foobar"}});
      });
    });
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10000000));
}
