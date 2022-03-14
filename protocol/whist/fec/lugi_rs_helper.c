
#include <string.h>
#include <SDL2/SDL_thread.h>
#include "lugi_rs_helper.h"
#include "rs_common.h"
#include <whist/logging/logging.h>

// size of row and column of RSTable
#define RS_TABLE_SIZE (RS_FIELD_SIZE + 1)

/*
============================
Globals
============================
*/

// Holds the RSTable for each thread
static SDL_TLSID rs_table_tls_id;

// This is the type for the rs_table we use for caching
typedef RSCode *RSTable[RS_TABLE_SIZE][RS_TABLE_SIZE];

/*
============================
Public Function Implementations
============================
*/

void lugi_rs_helper_init(void) {
    init_rs();
    rs_table_tls_id = SDL_TLSCreate();
}

RSCode *get_rs_code(int k, int n) {
    FATAL_ASSERT(k <= n);
    FATAL_ASSERT(n <= RS_FIELD_SIZE);

    // Get the rs code table for this thread
    RSTable *rs_code_table = SDL_TLSGet(rs_table_tls_id);

    // If the table for this thread doesn't exist, initialize it
    if (rs_code_table == NULL) {
        rs_code_table = (RSTable *)safe_malloc(sizeof(RSTable));
        memset(rs_code_table, 0, sizeof(RSTable));
        SDL_TLSSet(rs_table_tls_id, rs_code_table, free_rs_code_table);
    }

    // If (n, k)'s rs_code hasn't been create yet, create it
    if ((*rs_code_table)[k][n] == NULL) {
        (*rs_code_table)[k][n] = rs_new(k, n);
    }

    // Now return the rs_code for (n, k)
    return (RSCode *)((*rs_code_table)[k][n]);
    // We make a redundant (RSCode*) because cppcheck parses the type wrong
}

void free_rs_code_table(void *raw_rs_code_table) {
    RSTable *rs_code_table = (RSTable *)raw_rs_code_table;

    // If the table was never created, we have nothing to free
    if (rs_code_table == NULL) return;

    // Find any rs_code entries, and free them
    for (int i = 0; i < RS_TABLE_SIZE; i++) {
        for (int j = i; j < RS_TABLE_SIZE; j++) {
            if ((*rs_code_table)[i][j] != NULL) {
                rs_free((*rs_code_table)[i][j]);
            }
        }
    }

    // Now free the entire table
    free(rs_code_table);
}
