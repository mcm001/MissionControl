// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Plot.h"

#include <implot_internal.h>

#include <iostream>

#include <fmt/core.h>
#include <imgui.h>
#include <implot.h>
#include <networktables/NetworkTable.h>
#include <wpi/numbers>
#include <wpi/timestamp.h>
#include <fmt/format.h>

#include "WebSocketClient.h"

using namespace frc5190;

static double last_time;

ImPlotPoint ExtractPlotPoint(int idx, void* data) {
  return static_cast<ImPlotPoint*>(data)[idx];
}

Plot::Plot(std::shared_ptr<nt::NetworkTable> nt) : nt_{nt} {
  start_time_ = wpi::Now() * 1.0e-6;
  last_time = wpi::Now() * 1e-6;
}

double GetKey(wpi::json json, std::string_view key) {
  if(!json.is_object() || json.is_null()) return std::nan("");
  auto value = json[key];
  if(!json.is_number() || json.is_null()) return std::nan("");
  return (double) value;
}

void Plot::Display() {
  // Get current time.
  double now = wpi::Now() * 1.0e-6 - start_time_;

  // double fps = 1.0 / (now - last_time);
  // last_time = now;
  // fmt::print("FPS: {}\n", fps);

  // Add data to plot series.
  wpi::json current_json = WebSocketClient::GetInstance()->GetData();

  ker_tank_points[size_] = ImPlotPoint{now, GetKey(current_json, "kerTankDucer")};
  ker_venturi_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerVenturi", 0)};
  ker_inlet_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerInletDucer", 0)};
  ker_pintile_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerPintileDucer", 0)};
  lox_tank_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxTankDucer", 2)};
  lox_venturi_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxVenturi", 0)};
  lox_inlet_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxInletDucer", 1)};
  lox_pintile_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxPintileDucer", 4)};

  if (size_ < kMaxSize) {
    size_++;
  } else {
    offset_ = (offset_ + 1) % kMaxSize;
  }

  // Construct plots.
  // ImPlot::SetNextAxesLimits(now - kViewDuration, now, -20, 20,
  //                           ImPlotCond_Always);
  ImPlot::BeginSubplots("Ducers", 8, 2, ImVec2(-1, -1));

  for (int i = 0; i < 8; i++) {
    ImPlot::SetNextAxisLimits(ImAxis_X1, now - kViewDuration, now,
                              ImPlotCond_Always);
    if (ImPlot::BeginPlot("Kerosene Pressures")) {
      ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 1, 1));
      ImPlot::PlotLineG("Ker Tank", &ExtractPlotPoint, ker_tank_points, size_);

      ImPlot::SetNextLineStyle(ImVec4(0, 0.5f, 1, 1));
      ImPlot::PlotLineG("Ker Venturi", &ExtractPlotPoint, ker_venturi_points,
                        size_);

      // RGBA colors
      ImPlot::SetNextLineStyle(ImVec4(0, 0.3, 1, 1));
      ImPlot::PlotLineG("Ker Inlet", &ExtractPlotPoint, ker_inlet_points,
                        size_);

      ImPlot::SetNextLineStyle(ImVec4(0, 1, 0, 1));
      ImPlot::PlotLineG("Ker Pintile", &ExtractPlotPoint, ker_pintile_points,
                        size_);

      ImPlot::EndPlot();
    }


    ImPlot::SetNextAxisLimits(ImAxis_X1, now - kViewDuration, now,
                              ImPlotCond_Always);
    if (ImPlot::BeginPlot("Lox Pressures")) {
      ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 1, 1));
      ImPlot::PlotLineG("Lox Tank", &ExtractPlotPoint, lox_tank_points, size_);

      ImPlot::SetNextLineStyle(ImVec4(0, 0.5f, 1, 1));
      ImPlot::PlotLineG("Lox Venturi", &ExtractPlotPoint, lox_venturi_points,
                        size_);

      // RGBA colors
      ImPlot::SetNextLineStyle(ImVec4(0, 0.3, 1, 1));
      ImPlot::PlotLineG("Lox Inlet", &ExtractPlotPoint, lox_inlet_points,
                        size_);

      ImPlot::SetNextLineStyle(ImVec4(0, 1, 0, 1));
      ImPlot::PlotLineG("Lox Pintile", &ExtractPlotPoint, lox_pintile_points,
                        size_);

      ImPlot::EndPlot();
    }
  }

  ImPlot::EndSubplots();
}
