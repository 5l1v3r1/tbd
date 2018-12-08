//
//  include/recursive.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef RECURSIVE_H
#define RECURSIVE_H

#include <sys/types.h>
#include <stdio.h>

int
open_r(char *const path,
       const mode_t flags,
       const mode_t mode,
       char **const first_terminator_out);

int mkdir_r(char *path, mode_t mode, char **first_terminator_out);

/*
 * Remove only directories whose path-strings are formed when terminating
 * from and all subsequent slashes.
 */

int remove_parial_r(char *path, char *from);

#endif /* RECURSIVE_H */