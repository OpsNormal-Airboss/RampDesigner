#include "AppBadge.h"
#import <AppKit/AppKit.h>

namespace AppBadge {

void setCount(int count) {
    NSString* label = (count > 0)
        ? [NSString stringWithFormat:@"%d", count]
        : @"";
    [[NSApp dockTile] setBadgeLabel:label];
}

} // namespace AppBadge
