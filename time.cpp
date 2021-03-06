// tomato-clock
// Copyright © 2021 ailrk

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "time.h"
#include <cctype>
#include <tuple>

namespace tomatoclock {

const char *
show(precision_t pres) {
    switch (pres) {
    case precision_t::SEC:
        return "sec";
    case precision_t::MIN:
        return "min";
    case precision_t::HOUR:
        return "hour";
    }
}

uint32_t
to_s(precision_t pres) {
    switch (pres) {
    case precision_t::SEC:
        return 1;
    case precision_t::MIN:
        return 60;
    case precision_t::HOUR:
        return 60 * 60;
    }
}

inline static std::optional<uint32_t>
precision_range(precision_t upper, precision_t lower) {
    uint32_t diff =
        static_cast<uint32_t>(upper) - static_cast<uint32_t>(lower) + 1;
    if (diff < 1 || diff > 3) {
        return std::nullopt;
    }
    return diff;
}

std::optional<std::string>
mk_timestr(uint32_t sec, precision_t upper, precision_t lower) {
    auto precision_opt = precision_range(upper, lower);
    if (!precision_opt.has_value()) {
        fprintf(stderr, "unsupported date format %s, %s", show(upper),
                show(lower));
        return std::nullopt;
    }

    std::stringstream ss;

    auto precision_range = precision_opt.value();
    uint32_t u = static_cast<uint32_t>(upper);

    for (int i = 0; i < precision_range; ++i) {
        auto d = to_s(static_cast<precision_t>(u - i));
        uint32_t v = sec / d;
        sec -= v * d;
        ss << v;
        if (i != precision_range - 1) {
            ss << ":";
        }
    }

    return ss.str();
}

#define BUFSZ 3

std::optional<std::tuple<uint32_t, timefmt_t>>
parse_time_str(std::string_view time_str) {
    char buffer[3];
    char *sp = buffer;

    uint32_t sec = 0;
    uint32_t upper = 0;
    uint32_t lower = INT32_MAX;

    for (auto c : time_str) {
        if (std::isdigit(c)) {

            bool cond = sp - buffer > BUFSZ - 1;
            cond = cond || sp == buffer && c > '5';
            if (cond) {
                return std::nullopt;
            }
            *sp++ = c;
            continue;
        }

        if (sp > buffer + BUFSZ - 1) {
            return std::nullopt;
        }

        if (c != 's' && c != 'm' && c != 'h') {
            return std::nullopt;
        }

        *sp = '\0';
        sp = buffer;
        precision_t p;
        uint32_t v = std::stoi(buffer);
        switch (c) {
        case 's':
            sec += v;
            p = precision_t::SEC;
            break;
        case 'm':
            sec += v * 60;
            p = precision_t::MIN;
            break;
        case 'h':
            sec += v * 60 * 60;
            p = precision_t::HOUR;
            break;
        }

        uint32_t pint = static_cast<uint32_t>(p);
        if (pint > upper) {
            upper = pint;
        } else if (pint < lower) {
            lower = pint;
        }
    }
    return std::make_tuple(

        sec,

        timefmt_t{ .upper = static_cast<precision_t>(upper),
                   .lower = static_cast<precision_t>(lower) });
}
#undef BUFSZ

} // namespace tomatoclock
#ifdef TEST_TIME
#include <cassert>
using namespace tomatoclock;
int
main() {
    precision_t u1;
    precision_t l1;

    {
        u1 = precision_t::MIN;
        l1 = precision_t::SEC;
        auto v = tomatoclock::format_time(153, u1, l1);
        assert(v == "2:33");
    }

    {
        u1 = precision_t::HOUR;
        l1 = precision_t::SEC;
        auto v = tomatoclock::format_time(8000, u1, l1);
        assert(v == "2:13:20");
    }

    {
        auto [sec0, u0, l0] = tomatoclock::parse_time_str("20m10s").value();
        auto [sec1, u1, l1] = tomatoclock::parse_time_str("1h30m20s").value();
        assert(sec0 == 1210);
        assert(sec1 == 5420);
        assert(u0 == precision_t::MIN);
        assert(l0 == precision_t::SEC);
        assert(u1 == precision_t::HOUR);
        assert(l1 == precision_t::SEC);
    }

    return 0;
}
#endif
