// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include <chrono>
#include <string_view>
#include <thread>

#include <fmt/format.h>
#include <wpi/raw_ostream.h>
#include <wpinet/raw_uv_ostream.h>
#include <wpi/span.h>
#include <wpi/timestamp.h>
#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Process.h>
#include <wpinet/uv/Tcp.h>
#include <wpinet/uv/Timer.h>
#include <wpinet/uv/Udp.h>

#include "MyHttpConnection.h"

namespace uv = wpi::uv;

bool romi = false;
static uint64_t startTime = wpi::Now();

int main(int argc, char* argv[]) {
  int port = 9002;

  uv::Process::DisableStdioInheritance();

  auto loop = uv::Loop::Create();

  loop->error.connect(
      [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });

  auto tcp = uv::Tcp::Create(loop);

  // bind to listen address and port
  tcp->Bind("", port);

  auto timer2 = uv::Timer::Create(loop);

  // when we get a connection, accept it and start reading
  tcp->connection.connect([srv = tcp.get(), timer2] {
    auto tcp = srv->Accept();
    if (!tcp) return;
    fmt::print(stderr, "{}", "Got a TCP connection\n");

    // Close on error
    tcp->error.connect([s = tcp.get()](wpi::uv::Error err) {
      fmt::print(stderr, "stream error: {}\n", err.str());
      s->Close();
    });

    auto conn = std::make_shared<MyHttpConnection>(tcp);
    tcp->SetData(conn);

    timer2->Start(std::chrono::seconds(1), std::chrono::seconds(1));
    timer2->timeout.connect([conn] {
      conn->SendText("{\"kerTankDucer\": 4}");
    });
  });

  // start listening for incoming connections
  tcp->Listen();
  fmt::print(stderr, "Listening on port {}\n", port);

  // run loop
  loop->Run();
}
