#include "../inc/command.h"

void print_row(Row *row);

void print_leaf_node(void *node);

void print_constants() {
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void print_leaf_node(void *node) {
    uint32_t num_cells = *get_leaf_node_num_cells(node);
    printf("leaf (size %d)\n", num_cells);
    for (uint32_t i = 0; i < num_cells; i++) {
        uint32_t key = *get_leaf_node_key(node, i);
        printf("  - %d : %d\n", i, key);
    }
}

MetaCommandResult do_meta_command(const InputBuffer *input_buffer, Table *table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        db_close(table);
        exit(EXIT_SUCCESS);
    } else if (strcmp(input_buffer->buffer, ".constants") == 0) {
        printf("Constants: \n");
        print_constants();
        return META_COMMAND_SUCCESS;
    } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
        printf("Tree:\n");
        print_leaf_node(get_page(table->pager, 0));
        return META_COMMAND_SUCCESS;
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_insert(const InputBuffer *input_buffer, Statement *statement) {
    statement->type = STATEMENT_INSERT;
    char *keyword = strtok(input_buffer->buffer, " ");
    char *id_string = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");

    assert (strcmp(keyword, "insert") == 0);

    // check validity of id
    if (id_string == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }
    long id = strtol(id_string, NULL, 10);
    if (id < 0) {
        return PREPARE_NEGATIVE_ID;
    }

    // check validity of username
    if (strlen(username) > COLUMN_USERNAME_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    // check validity of email
    if (strlen(email) > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = (uint32_t) id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement) {
    if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        return prepare_insert(input_buffer, statement);
    }
    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement *statement, Table *table) {
    void *node = get_page(table->pager, table->root_page_num);
    if (*get_leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS) {
        return EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = &(statement->row_to_insert);
    Cursor *cursor = table_end(table);

    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(const Statement *statement, Table *table) {
    assert(statement->type == STATEMENT_SELECT);

    Cursor *cursor = table_start(table);
    Row row;
    while (!(cursor->end_of_table)) {
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }
    free(cursor);
    return EXECUTE_SUCCESS;
}

void print_row(Row *row) {
    printf("%d %s %s\n", row->id, row->username, row->email);
}

ExecuteResult execute_statement(Statement *statement, Table *table) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
    }
}

