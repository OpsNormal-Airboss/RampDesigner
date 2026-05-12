#pragma once

/// Platform-native application badge (dock/taskbar violation count).
/// Implementation is in AppBadge_mac.mm (macOS) or AppBadge_win.cpp (Windows).
/// On other platforms the functions are no-ops.
namespace AppBadge {

/// Set the application badge to @p count.
/// Pass 0 to clear the badge.
void setCount(int count);

} // namespace AppBadge
