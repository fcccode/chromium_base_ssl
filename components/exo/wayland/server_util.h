// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_WAYLAND_SERVER_UTIL_H_
#define COMPONENTS_EXO_WAYLAND_SERVER_UTIL_H_

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>
#include <memory>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "components/exo/surface.h"
#include "ui/base/class_property.h"

struct wl_resource;

namespace display {
class Display;
}

namespace gfx {
class Insets;
class Rect;
class Size;
}  // namespace gfx

namespace exo {

class DataOffer;

namespace wayland {

template <class T>
T* GetUserDataAs(wl_resource* resource) {
  return static_cast<T*>(wl_resource_get_user_data(resource));
}

template <class T>
std::unique_ptr<T> TakeUserDataAs(wl_resource* resource) {
  std::unique_ptr<T> user_data = base::WrapUnique(GetUserDataAs<T>(resource));
  wl_resource_set_user_data(resource, nullptr);
  return user_data;
}

template <class T>
void DestroyUserData(wl_resource* resource) {
  TakeUserDataAs<T>(resource);
}

template <class T>
void SetImplementation(wl_resource* resource,
                       const void* implementation,
                       std::unique_ptr<T> user_data) {
  wl_resource_set_implementation(resource, implementation, user_data.release(),
                                 DestroyUserData<T>);
}

// Convert a timestamp to a time value that can be used when interfacing
// with wayland. Note that we cast a int64_t value to uint32_t which can
// potentially overflow.
uint32_t TimeTicksToMilliseconds(base::TimeTicks ticks);
uint32_t NowInMilliseconds();

wl_resource* GetSurfaceResource(Surface* surface);
void SetSurfaceResource(Surface* surface, wl_resource* resource);

wl_resource* GetDataOfferResource(const DataOffer* data_offer);
void SetDataOfferResource(DataOffer* data_offer,
                          wl_resource* data_offer_resource);

// Scale the |child_bounds| in such a way that if it should fill the
// |parent_size|'s width/height, it returns the |parent_size_in_pixel|'s
// width/height.
gfx::Rect ScaleBoundsToPixelSnappedToParent(
    const gfx::Size& parent_size_in_pixel,
    const gfx::Size& parent_size,
    float device_scale_factor,
    const gfx::Rect& child_bounds);

// Create the insets make sure that work area will be within the chrome's
// work area when converted to the pixel on client side.
// TODO(oshima): We should send these information in pixel so that
// client do not have to convert it back.
gfx::Insets GetAdjustedInsets(const display::Display& display);

}  // namespace wayland
}  // namespace exo

#endif  // COMPONENTS_EXO_WAYLAND_SERVER_UTIL_H_