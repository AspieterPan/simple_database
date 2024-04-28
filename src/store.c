#include "../include/store.h"

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = TABLE_MAX_PAGES * ROWS_PER_PAGE;

void pager_flush(const Pager *pager, uint32_t page_num, uint32_t size);

void *get_page(Pager *pager, uint32_t page_num);

void serialize_row(const Row *source, void *destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(const void *source, Row *destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void *row_slot(const Table *table, uint32_t row_num) {
    void *page = get_page(table->pager, row_num / ROWS_PER_PAGE);
    const uint32_t row_offset = row_num % ROWS_PER_PAGE;
    const uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

void *get_page(Pager *pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        printf("page_num out of range");
        exit(EXIT_FAILURE);
    }

    // 当 pager 当中的缓存没有命中时， 需要向文件读取对应的 page
    if (pager->pages[page_num] == NULL) {
        void *const page = malloc(PAGE_SIZE);

        // 文件中一共有多少页
        const uint32_t num_full_pages = pager->file_length / PAGE_SIZE;
        const uint32_t num_pages = (pager->file_length % PAGE_SIZE) ? (num_full_pages + 1) : (num_full_pages);

        if (page_num <= num_pages) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            const ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file: %d\n", errno);
            }
        }
        // todo: 这里似乎缺少了当 page_num > num_pages 时的处理

        pager->pages[page_num] = page;
    }
    return pager->pages[page_num];
}


Pager *pager_open(const char *filename) {
    const int fd = open(filename,
                        O_RDWR |      // Read/Write mode
                        +O_CREAT,  // Create file if it does not exist
                        +S_IWUSR |     // User write permission
                        +S_IRUSR   // User read permission
    );
    if (fd == -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    const off_t file_length = lseek(fd, 0, SEEK_END);

    Pager *pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;

    for (uint32_t i = 0; i < TABLE_MAX_ROWS; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}


Table *db_open(const char *filename) {
    Pager *pager = pager_open(filename);

    const uint32_t num_full_page = pager->file_length / PAGE_SIZE;
    const uint32_t num_rows_of_full_page = num_full_page * ROWS_PER_PAGE;
    const uint32_t num_rows =
            num_rows_of_full_page + (pager->file_length - num_full_page * PAGE_SIZE) / ROW_SIZE;

    Table *table = malloc(sizeof(Table));
    table->pager = pager;
    table->num_rows = num_rows;

    return table;
}


void db_close(Table *table) {
    Pager *pager = table->pager;

    // 处理所有满页
    const uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;
    for (uint32_t i = 0; i < num_full_pages; i++) {
        // 缓存没有命中过的 page， 直接跳过
        if (pager->pages[i] == NULL) {
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // 如果最后一页未满， 处理它
    const uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows) {
        const uint32_t num_page = num_full_pages;
        pager_flush(pager, num_page, num_additional_rows * ROW_SIZE);
        free(pager->pages[num_page]);
        pager->pages[num_page] = NULL;
    }

    // close file
    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file\n");
        exit(EXIT_FAILURE);
    }
    free(table);
}

void pager_flush(const Pager *pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush NULL page.\n");
        exit(EXIT_FAILURE);
    }

    const off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    const ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);
    if (bytes_written == -1) {
        printf("Error writing :%d\n", errno);
        exit(EXIT_FAILURE);
    }
}
