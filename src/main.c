/*
 * This file is part of SimpleFS, an API course project
 * Copyright (c) 2017 Francesco Circhetta.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file main.c
 * @author Francesco Circhetta
 * @date 22 June 2017
 * @brief SimpleFS: progetto di Algoritmi e Principi dell'Informatica
 * @see http://home.deib.polimi.it/barenghi/lib/exe/fetch.php?media=teaching:api:consegna.pdf
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "simplefs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define RES_OK "ok\n"
#define RES_FAIL "no\n"
#define RES_READ(x) "contenuto %s\n", (x)
#define RES_WRITE(x) "ok %d\n", (x)
#define RES_FIND(x) "ok %s\n", (x)

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * Find resource by its path string
 */
node_t *enter_path(node_t *node, char *path, char **new_name) {
    node_t *tmp = NULL;
    char *cur_token = strtok(path, " /\n\r");
    char *next_token = strtok(NULL, "/\n\r");
    if (cur_token == NULL)
        return NULL; /* Empty path */
    /* Try to enter the path token by token */
    while (cur_token) {
        /* Enter only if current node is a dir */
        if (fs_get_type(node) == Dir) {
            if ((tmp = fs_find_in_dir(node, cur_token))) {
                /* Resource found, get next token */
                node = tmp;
                cur_token = next_token;
                next_token = strtok(NULL, "/\n\r");
            } else {
                /* Resource not found */
                if (new_name != NULL && next_token == NULL) {
                    *new_name = cur_token;
                    break;
                }
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    return node;
}

/**
 * create <path>
 * create_dir <path>
 * Create a new empty file/directory
 */
void do_create(node_t *node, uint8_t type) {
    char *name = NULL;
    int res = -1;
    node = enter_path(node, NULL, &name);
    if (name != NULL)
        res = fs_create(node, name, type);

    printf(res == 0 ? RES_OK : RES_FAIL);
}

/**
 * read <path>
 * Read file content
 */
void do_read(node_t *node) {
    char *content;
    node = enter_path(node, NULL, NULL);
    if (node != NULL && (content = fs_get_file_content(node))) {
        printf(RES_READ(content));
        return;
    }
    printf(RES_FAIL);
}

/**
 * write <path> "<content>"
 * Write the whole file content
 */
void do_write(node_t *node) {
    char *path, *new_content;

    path = strtok(NULL, " \n\r"); /* First token is path */
    new_content = strtok(NULL, "\"\n\r"); /* Second token is content */

    node = enter_path(node, path, NULL);
    if (node != NULL
        && new_content != NULL
        && fs_set_file_content(node, new_content) == 0) {
        printf(RES_WRITE((int) strlen(new_content)));
        return;
    }
    printf(RES_FAIL);
}

/**
 * delete <path>
 * delete_r <path>
 * Delete a resource (also recursively)
 */
void do_delete(node_t *node, int recursive) {
    int res = -1;
    node = enter_path(node, NULL, NULL);
    if (node != NULL) {
        if (recursive) {
            fs_delete_r(node);
            res = 0;
        } else {
            res = fs_delete(node);
        }
    }
    printf(res == 0 ? RES_OK : RES_FAIL);
}

/**
 * find <name>
 * Find a resource in the entire FS
 */
void do_find(node_t *root) {
    char *token = strtok(NULL, " \n\r");
    size_t nres = 0;
    node_t **res = fs_find_r(root, token, &nres, NULL);
    if(nres > 0) {
        char **paths = malloc_or_die(nres * sizeof(char *));
        for (int i = 0; i < nres; i++) {
            paths[i] = fs_get_path(res[i]);
        }
        free(res);
        qsort(paths, nres, sizeof(char *), compare_str);
        for(int i = 0; i < nres; i++) {
            printf(RES_FIND(paths[i]));
            free(paths[i]);
        }
        free(paths);
    } else {
        printf(RES_FAIL);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int main() {
    char *line = NULL;
    /* Root node init */
    node_t *root = fs_new_root();
    /* Command parser */
    line = my_getline();
    while (*line != '\0') {
        char *token = strtok(line, " \n\r");
        if (token) {
            if (strcmp(token, "create") == 0) {
                do_create(root, File);
            } else if (strcmp(token, "create_dir") == 0) {
                do_create(root, Dir);
            } else if (strcmp(token, "read") == 0) {
                do_read(root);
            } else if (strcmp(token, "write") == 0) {
                do_write(root);
            } else if (strcmp(token, "delete") == 0) {
                do_delete(root, 0);
            } else if (strcmp(token, "delete_r") == 0) {
                do_delete(root, 1);
            } else if (strcmp(token, "find") == 0) {
                do_find(root);
            } else if (strcmp(token, "exit") == 0) {
                break;
            }
        }
        free(line);
        line = my_getline();
    }
    free(line);
    hashtable_destroy(root->payload.dirhash);
    free(root->name);
    free(root);
    return 0;
}
