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
#include "SystemStatus.h"

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

  // when we get a connection, accept it and start reading
  tcp->connection.connect([srv = tcp.get()] {
    auto tcp = srv->Accept();
    if (!tcp) return;
    fmt::print(stderr, "{}", "Got a connection\n");

    // Close on error
    tcp->error.connect([s = tcp.get()](wpi::uv::Error err) {
      fmt::print(stderr, "stream error: {}\n", err.str());
      s->Close();
    });

    auto conn = std::make_shared<MyHttpConnection>(tcp);
    tcp->SetData(conn);
  });

  // start listening for incoming connections
  tcp->Listen();

  fmt::print(stderr, "Listening on port {}\n", port);

  // start timer to collect system and vision status
  auto timer = uv::Timer::Create(loop);
  timer->Start(std::chrono::seconds(1), std::chrono::seconds(1));
  timer->timeout.connect([&loop] {
    SystemStatus::GetInstance()->UpdateAll();
  });


  // run loop
  loop->Run();
}
