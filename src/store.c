#include "../inc/store.h"

void pager_flush(const Pager *pager, uint32_t page_num);


void *get_leaf_node_cell(void *node, uint32_t cell_num);


void initialize_leaf_node(void *node);


/*
 * Row Layout
 */
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

/*
 * Page and Table Layout
 */
const uint32_t PAGE_SIZE = 4096;


/*
 * Common Node Header Layout
 */
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE =
        NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/*
 * Leaf Node Header Layout
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/*
 * Leaf Node Body Layout
 */
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET =
        LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS =
        LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;


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

void *get_page(Pager *pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        printf("page_num out of range");
        exit(EXIT_FAILURE);
    }

    // 当 pager 当中的缓存没有命中时， 需要向文件读取对应的 page
    if (pager->pages[page_num] == NULL) {
        void *const page = malloc(PAGE_SIZE);

        // 文件中一共有多少页
        const uint32_t num_pages = pager->file_length / PAGE_SIZE;

        if (page_num < num_pages) {
            // 如果命中db文件中存在的Page, 则读取文件中对应的Page
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            const ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file: %d\n", errno);
            }
        } else {
            // 如果申请了超出db文件以外的页数, 则将超出部分全部作为空白页
            pager->num_pages = page_num + 1;
        }

        pager->pages[page_num] = page;
    }
    return pager->pages[page_num];
}

Pager *pager_open(const char *filename) {
    const int fd = open(filename, O_RDWR |      // Read/Write mode
                                  +O_CREAT, // Create file if it does not exist
                        +S_IWUSR |    // User write permission
                        +S_IRUSR  // User read permission
    );
    if (fd == -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    const off_t file_length = lseek(fd, 0, SEEK_END);

    Pager *pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = file_length / PAGE_SIZE;

    for (uint32_t i = 0; i < pager->num_pages; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

Table *db_open(const char *filename) {
    Pager *pager = pager_open(filename);

    Table *table = malloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        // New database file
        void *root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
    }

    return table;
}

void db_close(Table *table) {
    Pager *pager = table->pager;

    for (uint32_t i = 0; i < pager->num_pages; i++) {
        // 缓存没有命中过的 page， 直接跳过
        if (pager->pages[i] == NULL) {
            continue;
        }
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // close file
    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file\n");
        exit(EXIT_FAILURE);
    }
    free(table);
}

Cursor *table_start(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    // 令 cursor 指向 root_page 中第一个 cell
    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *get_leaf_node_num_cells(root_node);
    cursor->end_of_table = num_cells == 0;
    return cursor;
}

Cursor *table_end(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->end_of_table = true;

    // 令 cursor 指向 root_page 中最后一个 cell
    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *get_leaf_node_num_cells(root_node);
    cursor->cell_num = num_cells;
    return cursor;
}

void pager_flush(const Pager *pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush NULL page.\n");
        exit(EXIT_FAILURE);
    }

    const off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    const ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
    if (bytes_written == -1) {
        printf("Error writing :%d\n", errno);
        exit(EXIT_FAILURE);
    }
}

void *cursor_value(Cursor *cursor) {
    void *page = get_page(cursor->table->pager, cursor->page_num);
    return get_leaf_node_value(page, cursor->cell_num);
}

// cursor 位置前进 1 行
void cursor_advance(Cursor *cursor) {
    assert(!cursor->end_of_table);

    uint32_t page_num = cursor->page_num;
    void *node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;
    if (cursor->cell_num >= *get_leaf_node_num_cells(node)) {
        cursor->end_of_table = true;
    }
}

uint32_t *get_leaf_node_num_cells(void *node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void *get_leaf_node_cell(void *node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + LEAF_NODE_CELL_SIZE * cell_num;
}

uint32_t *get_leaf_node_key(void *node, uint32_t cell_num) {
    return get_leaf_node_cell(node, cell_num);
}

void *get_leaf_node_value(void *node, uint32_t cell_num) {
    return get_leaf_node_key(node, cell_num) + LEAF_NODE_KEY_SIZE;
}


void initialize_leaf_node(void *node) {
    *get_leaf_node_num_cells(node) = 0;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
    void *node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *get_leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        // node full
        printf("Need to implement splitting a leaf node.\n");
        exit(EXIT_FAILURE);
    }

    if (cursor->cell_num < num_cells) {
        //Make room for new cell
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(get_leaf_node_cell(node, i), get_leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }
    *(get_leaf_node_num_cells(node)) += 1;
    *(get_leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, get_leaf_node_value(node, cursor->cell_num));
}