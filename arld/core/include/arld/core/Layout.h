#pragma once

namespace arld::core {

class Layout {
public:
    Layout() = default;
    ~Layout() = default;

    Layout(const Layout&) = delete;
    Layout& operator=(const Layout&) = delete;
    Layout(Layout&&) = default;
    Layout& operator=(Layout&&) = default;
};

} // namespace arld::core
