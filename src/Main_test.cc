#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Process.h>
#include <wpinet/uv/Tcp.h>
#include <wpinet/uv/Timer.h>
#include <wpinet/WebSocket.h>
#include <wpinet/uv/Udp.h>
#include <wpi/Logger.h>
#include <wpinet/ParallelTcpConnector.h>

namespace uv = wpi::uv;

std::shared_ptr<uv::Loop> loop;

void Finish() {
  loop->Walk([](uv::Handle& it) { it.Close(); });
}

int main() {
  loop = uv::Loop::Create();

  loop->error.connect(
      [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

  auto tcp = uv::Tcp::Create(loop);
  const char* pipeName = "localhost"; const int port = 9002;

  // Our callback should be called when the TCP connection opens
  tcp->Connect(pipeName, port, [&] {
    auto ws = wpi::WebSocket::CreateClient(*tcp, "/test", pipeName);
    ws->closed.connect([&](uint16_t code, std::string_view reason) {
      Finish();
      if (code != 1005 && code != 1006) {
        // FAIL() << "Code: " << code << " Reason: " << reason;
      }
    });
    ws->open.connect([&](std::string_view protocol) {
      printf("Got open!\n");
      Finish();
    });
  });

  printf("Running loop\n");
  loop->Run();
  printf("Loop ran!\n");
}
