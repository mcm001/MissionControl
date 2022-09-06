// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include <glass/View.h>
#include <implot.h>
#include <networktables/NetworkTable.h>

namespace frc5190 {
class Plot : public glass::View {
 public:
  static constexpr int kMaxSize = 20000;
  static constexpr int kViewDuration = 10;

  explicit Plot(std::shared_ptr<nt::NetworkTable> nt);
  void Display() override;

 private:
  std::shared_ptr<nt::NetworkTable> nt_;

  double start_time_;

  ImPlotPoint ker_tank_points[kMaxSize];
  ImPlotPoint ker_venturi_points[kMaxSize];
  ImPlotPoint ker_inlet_points[kMaxSize];
  ImPlotPoint ker_pintile_points[kMaxSize];

  ImPlotPoint lox_tank_points[kMaxSize];
  ImPlotPoint lox_venturi_points[kMaxSize];
  ImPlotPoint lox_inlet_points[kMaxSize];
  ImPlotPoint lox_pintile_points[kMaxSize];

  int size_ = 0;
  int offset_ = 0;
};
}  // namespace frc5190
