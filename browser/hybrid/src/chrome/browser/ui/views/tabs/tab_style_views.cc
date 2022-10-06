// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
// Redefine Brave's patch to allow method overloading
#define BRAVE_GM2_TAB_STYLE_H \
  protected:

#define CreateForTab CreateForTab_ChromiumImpl
#include "src/chrome/browser/ui/views/tabs/tab_style_views.cc"
#undef CreateForTab
#undef BRAVE_GM2_TAB_STYLE_H

namespace {
class WhistGM2TabStyle : public GM2TabStyle {
  public:
    explicit WhistGM2TabStyle(Tab* tab);
    WhistGM2TabStyle(const WhistGM2TabStyle&) = delete;
    WhistGM2TabStyle& operator=(WhistGM2TabStyle&) = delete;
  protected:
    void PaintTabBackground(gfx::Canvas* canvas,
                            TabActive active,
                            absl::optional<int> fill_id,
                            int y_inset) const override;
    void PaintBottomBar(gfx::Canvas* canvas, SkColor color) const;
};

WhistGM2TabStyle::WhistGM2TabStyle(Tab* tab)
    : GM2TabStyle(tab) {}

void WhistGM2TabStyle::PaintBottomBar(gfx::Canvas* canvas, SkColor color) const {
  const float scale = canvas->image_scale();
  const int stroke_thickness = 2;
  gfx::RectF aligned_bounds = ScaleAndAlignBounds(tab_->bounds(), scale, stroke_thickness);
  const float extension = GetCornerRadius() * scale;
  const float left = aligned_bounds.x();
  const float right = aligned_bounds.right();
  const float extended_bottom = aligned_bounds.bottom();
  const float bottom_extension = GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP) * scale;

  const float stroke_adjustment = stroke_thickness * scale;
  float tab_left = left + extension + 2 * stroke_adjustment;
  float tab_right = right - extension - 2 * stroke_adjustment;
  float tab_bottom = extended_bottom - bottom_extension - stroke_adjustment;

  SkPath path;
  path.incReserve(4); // Upper bound to prevent mallocs at every step
  path.moveTo(tab_left, tab_bottom);
  path.lineTo(tab_right, tab_bottom);

  gfx::PointF origin(tab_->origin());
  origin.Scale(scale);
  path.offset(-origin.x(), -origin.y());

  // This is important to make sure the UndoDeviceScaleFactor does not persist
  // to the next draw call. This causes a ghost image of the unscaled path if
  // not present.
  gfx::ScopedCanvas scoped_canvas(canvas);
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(color); // Or see setShader and cc::PaintShader::MakeLinearGradient ;) --roshan
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(stroke_thickness * canvas->UndoDeviceScaleFactor());
  // By default it's kButt_Cap, which is a square line end.
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  canvas->DrawPath(path, flags);
}

void WhistGM2TabStyle::PaintTabBackground(gfx::Canvas* canvas,
                                          TabActive active,
                                          absl::optional<int> fill_id,
                                          int y_inset) const {
  GM2TabStyle::PaintTabBackground(canvas, active, fill_id, y_inset);
  // Draw bottom bar if this is not a throb pass and this is a cloud tab
  if (tab_->IsCloudTab()) {
    PaintBottomBar(canvas, SkColorSetRGB(0x71, 0xD9, 0xFF));
  }
  PaintSeparators(canvas);
}
}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  return std::make_unique<WhistGM2TabStyle>(tab);
}
