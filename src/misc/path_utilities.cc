//
//  src/misc/path_utilities.cc
//  tbd
//
//  Created by inoahdev on 9/9/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "path_utilities.h"

namespace path {
    char *find_next_slash(char *string) {
        auto iter = string;
        auto elmt = *iter;

        while (elmt != '\0' && elmt != '/' && elmt != '\\') {
            elmt = *(iter++);;
        }

        if (!elmt) {
            return nullptr;
        }

        return iter;
    }

    const char *find_next_slash(const char *string) {
        auto iter = string;
        auto elmt = *iter;

        while (elmt != '\0' && elmt != '/' && elmt != '\\') {
            elmt = *(iter++);;
        }

        if (!elmt) {
            return nullptr;
        }

        return iter;
    }

    char *find_next_unique_slash(char *string) {
        auto iter = string;
        auto elmt = *iter;

        while (elmt != '\0' && elmt != '/' && elmt != '\\') {
            elmt = *(iter++);;
        }

        if (elmt != '\0') {
            elmt = iter[1];
            while (elmt != '\0' && (elmt == '/' || elmt == '\\')) {
                elmt = *(iter++);;
            }

            if (elmt == '\0') {
                return nullptr;
            }
        }

        return iter;
    }

    const char *find_next_unique_slash(const char *string) {
        auto iter = string;
        auto elmt = *iter;

        while (elmt != '\0' && elmt != '/' && elmt != '\\') {
            elmt = *(iter++);;
        }

        if (elmt != '\0') {
            elmt = iter[1];
            while (elmt != '\0' && (elmt == '/' || elmt == '\\')) {
                elmt = *(iter++);;
            }

            if (elmt == '\0') {
                return nullptr;
            }
        }

        return iter;
    }
}
