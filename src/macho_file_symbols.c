//
//  src/macho_fd_symbols.c
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "c_str_utils.h"
#include "mach-o/nlist.h"

#include "arch_info.h"
#include "macho_file_symbols.h"
#include "swap.h"
#include "tbd.h"

static enum macho_file_parse_result
handle_symbol(struct tbd_create_info *const info,
              const uint64_t arch_bit,
              const char *const string,
              const uint32_t n_desc,
              const uint32_t n_type,
              const uint64_t options)
{
    /*
     * Figure out the symbol-type from the symbol-string and desc.
     * Also ensure the symbol is exported externally, unless otherwise stated.
     */
    
    enum tbd_export_type symbol_type = TBD_EXPORT_TYPE_NORMAL_SYMBOL;
    const char *symbol_string = string;

    if (n_desc & N_WEAK_DEF) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_type = TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL;
    } else if (strncmp(symbol_string, "_OBJC_CLASS_$", 13) == 0) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_string += 13;
        symbol_type = TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL; 
    } else if (strncmp(symbol_string, "_OBJC_METACLASS_$",17) == 0) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_string += 17;
        symbol_type = TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL;
    } else if (strncmp(symbol_string, ".objc_class_name", 16) == 0) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_string += 16;
        symbol_type = TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL;
    } else if (strncmp(symbol_string, "_OBJC_IVAR_$", 12) == 0) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_string += 12;
        symbol_type = TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL; 
    } else {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }
    }
    
    struct tbd_export_info export_info = {
        .archs = arch_bit,
        .length = strlen(symbol_string),
        .string = (char *)symbol_string,
        .type = symbol_type,
    };

    struct array *const exports = &info->exports;
    struct array_cached_index_info cached_info = {};

    struct tbd_export_info *const existing_info =
        array_find_item_in_sorted(exports,
                                  sizeof(export_info),
                                  &export_info,
                                  tbd_export_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        existing_info->archs |= arch_bit;
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Add our symbol-info to the list, as a matching symbol-info was not found.
     *
     * Note: As the symbol is from a large allocation in the call hierarchy that
     * will eventually be freed, we need to allocate a copy of the symbol before
     * placing it in the list.
     */

    export_info.string = strndup(symbol_string, export_info.length);
    if (export_info.string == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }
    
    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(exports,
                                              sizeof(export_info),
                                              &export_info,
                                              &cached_info,
                                              NULL);

    if (add_export_info_result != E_ARRAY_OK) {
        free(export_info.string);
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}
            
enum macho_file_parse_result
macho_file_parse_symbols(struct tbd_create_info *const info,
                         const int fd,
                         const uint64_t arch_bit,
                         const uint64_t start,
                         const uint64_t size,
                         const bool is_big_endian,
                         const uint32_t symoff,
                         const uint32_t nsyms,
                         const uint32_t stroff,
                         const uint32_t strsize,
                         const uint64_t parse_options,
                         const uint64_t options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const uint32_t symbol_table_size = sizeof(struct nlist) * nsyms;
    if (symbol_table_size / sizeof(struct nlist) != nsyms) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint32_t string_table_end = stroff + strsize;
    if (size != 0) {
        if (string_table_end > size) {
            return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
        }
    }

    if (lseek(fd, start + symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist *const symbol_table = calloc(1, symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (lseek(fd, start + stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = calloc(1, strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (read(fd, string_table, strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const struct nlist *nlist = symbol_table;
    const struct nlist *const end = symbol_table + nsyms;

    for (; nlist != end; nlist++) {
        uint32_t n_type = nlist->n_type;
        uint32_t n_desc = nlist->n_desc;
        uint32_t index = nlist->n_un.n_strx;

        if (is_big_endian) {
            n_type = swap_uint32(n_type);
            n_desc = swap_uint32(n_desc);
            index = swap_uint32(index);
        }

        /*
         * Ensure that each symbol connects back to __TEXT, or is an indirect
         * symbol.
         */

        const uint32_t type = n_type & N_TYPE;
        if (type != N_SECT && type != N_INDR) {
            continue;
        }

        /*
         * For leniency reasons, ignore invalid symbol-references instead of
         * erroring out.
         */

        if (index >= strsize) {
            continue;
        }

        const char *const symbol_string = string_table + index;
        const uint32_t string_length =
            strnlen(symbol_string, strsize - index);

        /*
         * Ignore empty strings.
         */

        if (string_length == 0) {
            continue;
        }

        /*
         * Ignore strings that are just whitespace.
         */    

        if (c_str_with_len_is_all_whitespace(symbol_string, string_length)) {
            continue;
        }

        const enum macho_file_parse_result handle_symbol_result =
            handle_symbol(info,
                          arch_bit,
                          symbol_string,
                          n_desc,
                          n_type,
                          parse_options);

        if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
            free(symbol_table);
            free(string_table);

            return handle_symbol_result;
        }
    }

    free(symbol_table);
    free(string_table);

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_64(struct tbd_create_info *const info,
                            const int fd,
                            const uint64_t arch_bit,
                            const uint64_t start,
                            const uint64_t size,
                            const bool is_big_endian,
                            const uint32_t symoff,
                            const uint32_t nsyms,
                            const uint32_t stroff,
                            const uint32_t strsize,
                            const uint64_t parse_options,
                            const uint64_t options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const uint32_t symbol_table_size = sizeof(struct nlist_64) * nsyms;
    if (symbol_table_size / sizeof(struct nlist_64) != nsyms) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint32_t string_table_end = stroff + strsize;
    if (size != 0) {
        if (string_table_end > size) {
            return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
        }
    }

    if (lseek(fd, start + symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist_64 *const symbol_table = calloc(1, symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (lseek(fd, start + stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = calloc(1, strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (read(fd, string_table, strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const struct nlist_64 *nlist = symbol_table;
    const struct nlist_64 *const nlist_end = symbol_table + nsyms;

    for (; nlist != nlist_end; nlist++) {
        uint32_t n_type = nlist->n_type;
        uint32_t n_desc = nlist->n_desc;
        uint32_t index = nlist->n_un.n_strx;

        if (is_big_endian) {
            n_type = swap_uint32(n_type);
            n_desc = swap_uint32(n_desc);
            index = swap_uint32(index);
        }

        /*
         * Ensure that each symbol connects back to __TEXT, or is an indirect
         * symbol.
         */

        const uint32_t type = n_type & N_TYPE;
        if (type != N_SECT && type != N_INDR) {
            continue;
        }

        /*
         * For leniency reasons, ignore invalid symbol-references instead of
         * erroring out.
         */

        if (index >= strsize) {
            continue;
        }

        const char *symbol_string = string_table + index;
        const uint32_t symbol_string_length =
            strnlen(symbol_string, strsize - index);

        /*
         * Ignore empty strings.
         */

        if (symbol_string_length == 0) {
            continue;
        }

        /*
         * Ignore strings that are just whitespace.
         */    

        if (c_str_is_all_whitespace(symbol_string)) {
            continue;
        }

        const enum macho_file_parse_result handle_symbol_result =
            handle_symbol(info,
                          arch_bit,
                          symbol_string,
                          n_desc,
                          n_type,
                          parse_options);

        if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
            free(symbol_table);
            free(string_table);

            return handle_symbol_result;
        }
    }

    free(symbol_table);
    free(string_table);
    
    return E_MACHO_FILE_PARSE_OK;
}