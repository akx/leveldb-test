#include <leveldb/c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_TEST_ENTS 5000000
#define PROGRESS_INTERVAL (N_TEST_ENTS >> 4)

uint32_t z, w;

static uint32_t mwc() {
    uint32_t znew = (z=36969*(z&65535)+(z>>16));
    uint32_t wnew = (w=18000*(w&65535)+(w>>16));
    return (znew << 16) + wnew;
}

static void itoa(char *buf, uint32_t val, const int base) {
    int i = 30;
    memset(buf, 0, 32);
    for(; val && i ; --i, val /= base) {
        buf[i] = "0123456789abcdef"[val % base];
    }
}

//////////////////////////////////////////////////////////////////////

static leveldb_t *create_db() {
    char *err = NULL;
    leveldb_options_t *options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);
    leveldb_options_set_compression(options, 1);
    leveldb_t *db = leveldb_open(options, "testdb", &err);
    if(err) exit(4);
    return db;
}


static const char* hash_integer(const uint32_t key) {
    // Warning: this uses a static buffer;
    // please be sure to copy the return value somewhere
    // before re-calling.
    static char buf[32];
    itoa(buf, key, 16);
    return buf;
}

static void write_entries(leveldb_t *db) {
    char *err = NULL;
    leveldb_writeoptions_t *woptions;
    woptions = leveldb_writeoptions_create();
    leveldb_writeoptions_set_sync(woptions, 0);
    for(uint32_t i = 0; i < N_TEST_ENTS; i++) {
        char value[4];
        snprintf(value, 4, "%04x", i ^ 0xBADC0DE);
        leveldb_put(db, woptions, hash_integer(i), 32, value, 4, &err);
        if(err) exit(2);
        if(i % PROGRESS_INTERVAL == 0) {
            printf("%d...\n", i);
        }
    }
}

static void read_entries(leveldb_t *db, const int n) {
    leveldb_readoptions_t *roptions = leveldb_readoptions_create();
    char *read, *err;
    size_t read_len;
    for(uint32_t i = 0; i < n; i++) {
        err = NULL;
        const char *key = hash_integer(mwc() % N_TEST_ENTS);
        read = leveldb_get(db, roptions, key, 32, &read_len, &err);
        if(!read) exit(4);
        if(err) exit(3);
        leveldb_free(read);
    }
}

int main(int argc, char **argv) {
    leveldb_t *db = create_db();
    char mode[16] = "";

    // Marsaglia RNG initialization
    z = time(NULL);
    w = z ^ 0xCAFECAFE;

    if(argc > 1) {
        strncpy(mode, argv[1], 16);
    }
    if(strcmp(mode, "write") == 0) {
        printf("Writing entries...\n");
        write_entries(db);
        printf("Done.\n");
    }
    if(strcmp(mode, "read") == 0) {
        int n = 0;
        if(argc > 2) n = strtoul(argv[2], NULL, 10);
        if(!n) n = 1000000;
        printf("Reading %d random entries...\n", n);
        read_entries(db, n);
        printf("Done.\n");
    }
    leveldb_close(db);
}
