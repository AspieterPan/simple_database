#ifndef SIMPLE_DATABASE_STORE_H
#define SIMPLE_DATABASE_STORE_H

#include <printf.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

#define TABLE_MAX_PAGES 100

/*
 * Row Layout
 */
extern const uint32_t ID_SIZE;
extern const uint32_t USERNAME_SIZE;
extern const uint32_t EMAIL_SIZE;
extern const uint32_t ID_OFFSET;
extern const uint32_t USERNAME_OFFSET;
extern const uint32_t EMAIL_OFFSET;
extern const uint32_t ROW_SIZE;

/*
 * Page and Table Layout
 */
extern const uint32_t PAGE_SIZE;
#define TABLE_MAX_PAGES 100


typedef enum {
    NODE_LEAF,
    NODE_INTERNAL
} NodeType;

/*
 * Common Node Header Layout
 */
extern const uint32_t NODE_TYPE_SIZE;
extern const uint32_t NODE_TYPE_OFFSET;
extern const uint32_t IS_ROOT_SIZE;
extern const uint32_t IS_ROOT_OFFSET;
extern const uint32_t PARENT_POINTER_SIZE;
extern const uint32_t PARENT_POINTER_OFFSET;
extern const uint8_t COMMON_NODE_HEADER_SIZE;

/*
 * Leaf Node Header Layout
 */
extern const uint32_t LEAF_NODE_NUM_CELLS_SIZE;
extern const uint32_t LEAF_NODE_NUM_CELLS_OFFSET;
extern const uint32_t LEAF_NODE_HEADER_SIZE;

/*
 * Leaf Node Body Layout
 */
extern const uint32_t LEAF_NODE_KEY_SIZE;
extern const uint32_t LEAF_NODE_KEY_OFFSET;
extern const uint32_t LEAF_NODE_VALUE_SIZE;
extern const uint32_t LEAF_NODE_VALUE_OFFSET;
extern const uint32_t LEAF_NODE_CELL_SIZE;
extern const uint32_t LEAF_NODE_SPACE_FOR_CELLS;
extern const uint32_t LEAF_NODE_MAX_CELLS;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    uint32_t root_page_num;
    Pager *pager;
} Table;

typedef struct {
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates a position one past the last element
} Cursor;

void serialize_row(const Row *source, void *destination);

void deserialize_row(const void *source, Row *destination);

void *cursor_value(Cursor *cursor);

Pager *pager_open(const char *filename);

Table *db_open(const char *filename);

void db_close(Table *table);

/**
 *
 * @param table Table*
 * @return a Cursor pointing to the start of table
 */
Cursor *table_start(Table *table);

/**
 * @brief return a Cursor pointing to the end of table
 *
 * @param table Table*
 * @return a Cursor pointing to the end of table
 */
Cursor *table_end(Table *table);

/**
 * @brief cursor advances by one step
 *
 * @param cursor
 */
void cursor_advance(Cursor *cursor);

// 返回第 page_num 页的起始位置的指针
void *get_page(Pager *pager, uint32_t page_num);

uint32_t *get_leaf_node_num_cells(void *node);

void *get_leaf_node_value(void *node, uint32_t cell_num);

uint32_t *get_leaf_node_key(void *node, uint32_t cell_num);

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value);

#endif