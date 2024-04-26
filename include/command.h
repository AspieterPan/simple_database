#ifndef SIMPLE_DATABASE_COMMAND_H
#define SIMPLE_DATABASE_COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "../include/input_buffer.h"
#include "../include/store.h"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum {
    STATEMENT_INSERT, STATEMENT_SELECT
} StatementType;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

typedef enum {
    PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT, PREPARE_SYNTAX_ERROR, PREPARE_STRING_TOO_LONG, PREPARE_NEGATIVE_ID,
} PrepareResult;

typedef enum {
    EXECUTE_TABLE_FULL, EXECUTE_SUCCESS,
} ExecuteResult;

MetaCommandResult do_meta_command(InputBuffer *input_buffer);

PrepareResult prepare_statement(InputBuffer *input_buffer,
                                Statement *statement);

ExecuteResult execute_insert(Statement *statement, Table *table);

ExecuteResult execute_select(Statement *statement, Table *table);


ExecuteResult execute_statement(Statement *statement, Table *table);

#endif