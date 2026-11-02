#pragma once

#include "core/types.h"

namespace attendance {

void ShowStatsChart();
struct StatsChartLayout {
  RECT title{};
  std::array<RECT, 6> labels{}, bars{}, counts{};
  int contentHeight = 0;
};
StatsChartLayout MeasureStatsChartLayout(HWND hwnd, int width);

} // namespace attendance
