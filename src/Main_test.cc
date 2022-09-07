#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Process.h>
#include <wpinet/uv/Tcp.h>
#include <wpinet/uv/Timer.h>
#include <wpinet/uv/Udp.h>
#include <wpi/Logger.h>
#include <wpinet/ParallelTcpConnector.h>

namespace uv = wpi::uv;

int main() {
  auto loop = uv::Loop::Create();
  auto tcp = uv::TCP::Create(loop);
  const auto pipeName = "localhost:9002";
  tcp->Connect(pipeName, [&] {
    auto ws = WebSocket::CreateClient(*tcp, "/test", pipeName);
    ws->closed.connect([&](uint16_t code, std::string_view reason) {
      Finish();
      if (code != 1005 && code != 1006) {
        FAIL() << "Code: " << code << " Reason: " << reason;
      }
    });
    ws->open.connect([&](std::string_view protocol) {
      printf("Got open!\n");
      Finish();
    });
  });
}
