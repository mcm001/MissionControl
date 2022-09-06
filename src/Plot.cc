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

#include "Keys.h"

using namespace frc5190;

ImPlotPoint ExtractPlotPoint(void* data, int idx) {
  return static_cast<ImPlotPoint*>(data)[idx];
}

Plot::Plot(std::shared_ptr<nt::NetworkTable> nt) : nt_{nt} {
  start_time_ = wpi::Now() * 1.0e-6;
}

void Plot::Display() {
  // Get current time.
  double now = wpi::Now() * 1.0e-6 - start_time_;

  // Add data to plot series.
  ker_tank_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerTankDucer", 0)};
  ker_venturi_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerVenturi", 0)};
  ker_inlet_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerInletDucer", 0)};
  ker_pintile_points[size_] = ImPlotPoint{now, nt_->GetNumber("kerPintileDucer", 0)};
  lox_tank_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxTankDucer", 0)};
  lox_venturi_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxVenturi", 0)};
  lox_inlet_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxInletDucer", 0)};
  lox_pintile_points[size_] = ImPlotPoint{now, nt_->GetNumber("loxPintileDucer", 0)};

  if (size_ < kMaxSize) {
    size_++;
  } else {
    offset_ = (offset_ + 1) % kMaxSize;
  }

  // Construct plots.
  // ImPlot::SetNextAxesLimits(now - kViewDuration, now, -20, 20,
  //                           ImPlotCond_Always);
  ImPlot::BeginSubplots("Ducers", 2, 2, ImVec2(-1, -1));

  ImPlot::SetNextAxisLimits(ImAxis_X1, now - kViewDuration, now, ImPlotCond_Always);
  if (ImPlot::BeginPlot("Kerosene Pressures")) {
    ImPlot::SetNextLineStyle(ImVec4(1,0.5f,1,1));
    ImPlot::PlotLineG("Ker Tank", &ExtractPlotPoint, ker_tank_points, size_);

    ImPlot::SetNextLineStyle(ImVec4(0,0.5f,1,1));
    ImPlot::PlotLineG("Ker Venturi", &ExtractPlotPoint, ker_venturi_points,size_);

    // RGBA colors
    ImPlot::SetNextLineStyle(ImVec4(0,0.3,1,1));
    ImPlot::PlotLineG("Ker Inlet", &ExtractPlotPoint, ker_inlet_points,size_);

    ImPlot::SetNextLineStyle(ImVec4(0,1,0,1));
    ImPlot::PlotLineG("Ker Pintile", &ExtractPlotPoint, ker_pintile_points,size_);

    ImPlot::EndPlot();
  }

  ImGui::SameLine();

  ImPlot::SetNextAxisLimits(ImAxis_X1, now - kViewDuration, now, ImPlotCond_Always);
  if (ImPlot::BeginPlot("Lox Pressures")) {
    ImPlot::SetNextLineStyle(ImVec4(1,0.5f,1,1));
    ImPlot::PlotLineG("Lox Tank", &ExtractPlotPoint, lox_tank_points, size_);

    ImPlot::SetNextLineStyle(ImVec4(0,0.5f,1,1));
    ImPlot::PlotLineG("Lox Venturi", &ExtractPlotPoint, lox_venturi_points,size_);

    // RGBA colors
    ImPlot::SetNextLineStyle(ImVec4(0,0.3,1,1));
    ImPlot::PlotLineG("Lox Inlet", &ExtractPlotPoint, lox_inlet_points,size_);

    ImPlot::SetNextLineStyle(ImVec4(0,1,0,1));
    ImPlot::PlotLineG("Lox Pintile", &ExtractPlotPoint, lox_pintile_points,size_);

    ImPlot::EndPlot();
  }

  ImPlot::SetNextAxisLimits(ImAxis_X1, now - kViewDuration, now, ImPlotCond_Always);
  if (ImPlot::BeginPlot("Kerosene Pressures")) {
    ImPlot::SetNextLineStyle(ImVec4(1,0.5f,1,1));
    ImPlot::PlotLineG("Ker Tank", &ExtractPlotPoint, ker_tank_points, size_);

    ImPlot::SetNextLineStyle(ImVec4(0,0.5f,1,1));
    ImPlot::PlotLineG("Ker Venturi", &ExtractPlotPoint, ker_venturi_points,size_);

    // RGBA colors
    ImPlot::SetNextLineStyle(ImVec4(0,0.3,1,1));
    ImPlot::PlotLineG("Ker Inlet", &ExtractPlotPoint, ker_inlet_points,size_);

    ImPlot::SetNextLineStyle(ImVec4(0,1,0,1));
    ImPlot::PlotLineG("Ker Pintile", &ExtractPlotPoint, ker_pintile_points,size_);

    ImPlot::EndPlot();
  }

  ImGui::SameLine();

  ImPlot::SetNextAxisLimits(ImAxis_X1, now - kViewDuration, now, ImPlotCond_Always);
  if (ImPlot::BeginPlot("Lox Pressures")) {
    ImPlot::SetNextLineStyle(ImVec4(1,0.5f,1,1));
    ImPlot::PlotLineG("Lox Tank", &ExtractPlotPoint, lox_tank_points, size_);

    ImPlot::SetNextLineStyle(ImVec4(0,0.5f,1,1));
    ImPlot::PlotLineG("Lox Venturi", &ExtractPlotPoint, lox_venturi_points,size_);

    // RGBA colors
    ImPlot::SetNextLineStyle(ImVec4(0,0.3,1,1));
    ImPlot::PlotLineG("Lox Inlet", &ExtractPlotPoint, lox_inlet_points,size_);

    ImPlot::SetNextLineStyle(ImVec4(0,1,0,1));
    ImPlot::PlotLineG("Lox Pintile", &ExtractPlotPoint, lox_pintile_points,size_);

    ImPlot::EndPlot();
  }
  ImPlot::EndSubplots();
}
