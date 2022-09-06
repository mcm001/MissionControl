// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <iostream>
#include <memory>

#include <fmt/core.h>
#include <glass/Context.h>
#include <glass/MainMenuBar.h>
#include <glass/Storage.h>
#include <glass/Window.h>
#include <glass/WindowManager.h>
#include <glass/networktables/NetworkTables.h>
#include <glass/networktables/NetworkTablesSettings.h>
#include <glass/networktables/NTField2D.h>
#include <glass/other/Field2D.h>
#include <imgui.h>
#include <networktables/NetworkTableEntry.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/TableEntryListener.h>
#include <wpigui.h>

#include "Plot.h"

#include "MyHttpConnection.h"
#include "WebSocketHandlers.h"
#include <wpinet/uv/Loop.h>
#include <wpinet/uv/Process.h>
#include <wpinet/uv/Tcp.h>
#include <wpinet/uv/Timer.h>
#include <wpinet/uv/Udp.h>
#include <wpi/Logger.h>
#include <wpinet/ParallelTcpConnector.h>

using namespace frc5190;

namespace gui = wpi::gui;

static std::unique_ptr<glass::WindowManager> window_manager_;

static glass::Window* plot_;

static std::unique_ptr<glass::NetworkTablesModel> gNetworkTablesModel;
static std::unique_ptr<glass::NetworkTablesSettings> gNetworkTablesSettings;
static std::unique_ptr<glass::Window> gNetworkTablesWindow;
static std::unique_ptr<glass::Window> gNetworkTablesSettingsWindow;

static glass::MainMenuBar main_menu_bar_;

namespace uv = wpi::uv;
static constexpr uv::Timer::Time kReconnectRate{1000};
static constexpr uv::Timer::Time kWebsocketHandshakeTimeout{500};

class GuiWebsocketClient {
public:
  std::shared_ptr<wpi::ParallelTcpConnector> m_parallelConnect;
  wpi::Logger m_logger;


  GuiWebsocketClient(uv::Loop& loop) {
    printf("Ctor\n");
    m_parallelConnect = wpi::ParallelTcpConnector::Create(
          loop, kReconnectRate, m_logger,
          [this](uv::Tcp& tcp) { TcpConnected(tcp); });
    printf("ctor2\n");

    // std::vector<std::pair<std::string, unsigned int>> m_servers = {{"localhost", 9002}};
    std::vector<std::pair<std::string, unsigned int>> m_servers = {};
    m_parallelConnect->SetServers(m_servers);
    printf("ctor3\n");
  }

  void ProcessIncomingText(std::string_view data) { }
  void ProcessIncomingBinary(wpi::span<const uint8_t> data) { }

  void Disconnect(std::string_view reason) { }

  void WsConnected(wpi::WebSocket& ws, uv::Tcp& tcp) {
    m_parallelConnect->Succeeded(tcp);

    // ConnectionInfo connInfo;
    // uv::AddrToName(tcp.GetPeer(), &connInfo.remote_ip, &connInfo.remote_port);
    // connInfo.protocol_version = 0x0400;

    ws.closed.connect([this, &ws](uint16_t, std::string_view reason) {
      if (!ws.GetStream().IsLoopClosing()) {
        Disconnect(reason);
      }
    });
    ws.text.connect([this](std::string_view data, bool) {
      ProcessIncomingText(data);
    });
    ws.binary.connect([this](wpi::span<const uint8_t> data, bool) {
      ProcessIncomingBinary(data);
    });
  }

  void TcpConnected(uv::Tcp& tcp) {
    tcp.SetNoDelay(true);
    // Start the WS client
    // if (m_logger.min_level() >= wpi::WPI_LOG_DEBUG4) {
    //   std::string ip;
    //   unsigned int port = 0;
    //   uv::AddrToName(tcp.GetPeer(), &ip, &port);
    //   DEBUG4("Starting WebSocket client on {} port {}", ip, port);
    // }
    wpi::WebSocket::ClientOptions options;
    options.handshakeTimeout = kWebsocketHandshakeTimeout;
    auto ws =
        wpi::WebSocket::CreateClient(tcp, fmt::format("/nt/{}", "foobar"), "",
                                    {{"networktables.first.wpi.edu"}}, options);
    ws->open.connect([this, &tcp, ws = ws.get()](std::string_view) {
      // Unclear if we need this, actually
      // if (m_connList.IsConnected()) {
      //   ws->Terminate(1006, "no longer needed");
      //   return;
      // }
      WsConnected(*ws, tcp);
    });
  }
};

#include <exception>

void Application(std::string_view save_dir) {

  // ====== WS stuff ========
  uv::Process::DisableStdioInheritance();
  auto loop = uv::Loop::Create();
  loop->error.connect(
    [](uv::Error err) { fmt::print(stderr, "uv ERROR: {}\n", err.str()); throw std::exception(); });

  // TODO I think this is causing issues
  GuiWebsocketClient client(loop);


  // // when we get a connection, accept it and start reading
  // tcp->connection.connect([srv = tcp.get()] {
  //   auto tcp = srv->Accept();
  //   if (!tcp) return;
  //   // fmt::print(stderr, "{}", "Got a connection\n");

  //   // Close on error
  //   tcp->error.connect([s = tcp.get()](wpi::uv::Error err) {
  //     fmt::print(stderr, "stream error: {}\n", err.str());
  //     s->Close();
  //   });

  //   auto conn = std::make_shared<MyHttpConnection>(tcp);
  //   tcp->SetData(conn);
  // });
  //   // start listening for incoming connections
  // tcp->Listen();
  // fmt::print(stderr, "Listening on port {}\n", port);
  loop->Run();


  return;

  // ============= Glass stuff =============

  // Create wpigui and Glass contexts.
  fmt::print("Starting Glass!\n");
  gui::CreateContext();
  glass::CreateContext();

  // Set the storage name and location.
  glass::SetStorageName("Mission Control");
  glass::SetStorageDir(save_dir.empty() ? gui::GetPlatformSaveFileDir()
                                        : save_dir);

  // Create global NT instance.
  nt::NetworkTableInstance inst = nt::NetworkTableInstance::GetDefault();
  // inst.StartClient("localhost");
  // inst.StartClientTeam(5190);

  std::shared_ptr<nt::NetworkTable> robot_table = inst.GetTable("fcb");

  // Initialize window manager and add views.
  auto& storage = glass::GetStorageRoot().GetChild("Mission Control");
  window_manager_ = std::make_unique<glass::WindowManager>(storage);
  window_manager_->GlobalInit();


  // NetworkTables table window
  gNetworkTablesModel = std::make_unique<glass::NetworkTablesModel>();
  gui::AddEarlyExecute([] { gNetworkTablesModel->Update(); });

  gNetworkTablesWindow = std::make_unique<glass::Window>(
      glass::GetStorageRoot().GetChild("NetworkTables View"), "NetworkTables");
  gNetworkTablesWindow->SetView(
      std::make_unique<glass::NetworkTablesView>(gNetworkTablesModel.get()));
  gNetworkTablesWindow->SetDefaultPos(250, 277);
  gNetworkTablesWindow->SetDefaultSize(750, 185);
  gNetworkTablesWindow->DisableRenamePopup();
  gui::AddLateExecute([] { gNetworkTablesWindow->Display(); });

  // NetworkTables settings window
  gNetworkTablesSettings = std::make_unique<glass::NetworkTablesSettings>(
      glass::GetStorageRoot().GetChild("NetworkTables Settings"));
  gui::AddEarlyExecute([] { gNetworkTablesSettings->Update(); });

  gNetworkTablesSettingsWindow = std::make_unique<glass::Window>(
      glass::GetStorageRoot().GetChild("NetworkTables Settings"),
      "NetworkTables Settings");
  gNetworkTablesSettingsWindow->SetView(
      glass::MakeFunctionView([] { gNetworkTablesSettings->Display(); }));
  gNetworkTablesSettingsWindow->SetDefaultPos(30, 30);
  gNetworkTablesSettingsWindow->SetFlags(ImGuiWindowFlags_AlwaysAutoResize);
  gNetworkTablesSettingsWindow->DisableRenamePopup();
  gui::AddLateExecute([] { gNetworkTablesSettingsWindow->Display(); });

  plot_ =
      window_manager_->AddWindow("Plot", std::make_unique<Plot>(robot_table));

  // Add menu bar.
  gui::AddLateExecute([&] {
    ImGui::BeginMainMenuBar();
    main_menu_bar_.WorkspaceMenu();
    gui::EmitViewMenu();
    window_manager_->DisplayMenu();
    ImGui::EndMainMenuBar();

    // field_model.Update();
  });

  // Initialize and run GUI.
  gui::Initialize("Mission Control", 1920, 1080);
  gui::Main();

  // Shutdown.
  glass::DestroyContext();
  gui::DestroyContext();
}

int main() {
  Application("");
  return 0;
}
