// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_clock.h"

#include "base/time/default_clock.h"

namespace offline_pages {

namespace {
base::Clock* custom_clock_ = nullptr;
}

base::Clock* OfflineClock() {
  if (custom_clock_)
    return custom_clock_;
  return base::DefaultClock::GetInstance();
}

void SetOfflineClockForTesting(base::Clock* clock) {
  custom_clock_ = clock;
}

}  // namespace offline_pages