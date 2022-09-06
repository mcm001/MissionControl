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

using namespace frc5190;

namespace gui = wpi::gui;

static std::unique_ptr<glass::WindowManager> window_manager_;

static glass::Window* plot_;

static std::unique_ptr<glass::NetworkTablesModel> gNetworkTablesModel;
static std::unique_ptr<glass::NetworkTablesSettings> gNetworkTablesSettings;
static std::unique_ptr<glass::Window> gNetworkTablesWindow;
static std::unique_ptr<glass::Window> gNetworkTablesSettingsWindow;

static glass::MainMenuBar main_menu_bar_;

void Application(std::string_view save_dir) {
  // Create wpigui and Glass contexts.
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
