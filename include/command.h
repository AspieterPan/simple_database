#ifndef META_COMMAND_H
#define META_COMMAND_H

#include "../include/input_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;

typedef struct {
  StatementType type;
} Statement;

typedef enum { PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT } PrepareResult;

MetaCommandResult do_meta_command(InputBuffer *input_buffer);

PrepareResult prepare_statement(InputBuffer *input_buffer,
                                Statement *statement);

void execute_statement(Statement *statement);

#endif