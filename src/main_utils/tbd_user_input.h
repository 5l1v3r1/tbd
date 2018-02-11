//
//  src/main_utils/tbd_user_input.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <iostream>
#include "tbd_with_options.h"

namespace main_utils {
    void request_input(std::string &string, const char *prompt, const std::initializer_list<const char *> &acceptable_inputs) noexcept;

    struct create_tbd_retained {
        explicit inline create_tbd_retained() = default;
        explicit inline create_tbd_retained(uint64_t value) : value(value) {}

        union {
            uint64_t value = 0;

            struct {
                bool replace_platform          : 1;
                bool replace_installation_name : 1;
                bool replace_objc_constraint   : 1;
                bool replace_swift_version     : 1;
                bool replace_parent_umbrella   : 1;
                bool ignore_flags              : 1;
                bool ignore_uuids              : 1;
                bool ignore_non_unique_uuids   : 1;
                bool ignore_missing_uuids      : 1;
            } __attribute__((packed));
        };

        inline bool has_none() const noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const create_tbd_retained &retained) const noexcept { return this->value == retained.value; }
        inline bool operator!=(const create_tbd_retained &retained) const noexcept { return this->value != retained.value; }
    };

    bool request_new_installation_name(tbd_with_options &all, tbd_with_options &tbd, create_tbd_retained *info) noexcept;
    bool request_new_objc_constraint(tbd_with_options &all,   tbd_with_options &tbd, create_tbd_retained *info) noexcept;
    bool request_new_parent_umbrella(tbd_with_options &all,   tbd_with_options &tbd, create_tbd_retained *info) noexcept;
    bool request_new_platform(tbd_with_options &all,          tbd_with_options &tbd, create_tbd_retained *info) noexcept;
    bool request_new_swift_version(tbd_with_options &all,     tbd_with_options &tbd, create_tbd_retained *info) noexcept;

    bool request_if_should_ignore_flags(tbd_with_options &all, tbd_with_options &tbd, create_tbd_retained *info) noexcept;
    bool request_if_should_ignore_uuids(tbd_with_options &all, tbd_with_options &tbd, create_tbd_retained *info) noexcept;

    bool request_if_should_ignore_non_unique_uuids(tbd_with_options &all, tbd_with_options &tbd, create_tbd_retained *info) noexcept;
    bool request_if_should_ignore_missing_uuids(tbd_with_options &all, tbd_with_options &tbd, create_tbd_retained *info) noexcept;
}
