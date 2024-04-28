#ifndef SIMPLE_DATABASE_STORE_H
#define SIMPLE_DATABASE_STORE_H

#include <string.h>
#include <sys/errno.h>
#include <unistd.h>
#include <printf.h>
#include <sys/file.h>
#include <stdlib.h>
#include <sys/types.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

#define TABLE_MAX_PAGES 100

extern const uint32_t ID_SIZE;
extern const uint32_t USERNAME_SIZE;
extern const uint32_t EMAIL_SIZE;
extern const uint32_t ID_OFFSET;
extern const uint32_t USERNAME_OFFSET;
extern const uint32_t EMAIL_OFFSET;
extern const uint32_t ROW_SIZE;

extern const uint32_t PAGE_SIZE;
extern const uint32_t ROWS_PER_PAGE;
extern const uint32_t TABLE_MAX_ROWS;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    uint32_t num_rows;
    Pager *pager;
} Table;

void serialize_row(const Row *source, void *destination);

void deserialize_row(const void *source, Row *destination);

void *row_slot(const Table *table, uint32_t row_num);

Pager *pager_open(const char *filename);

Table *db_open(const char *filename);

void db_close(Table *table);

#endif