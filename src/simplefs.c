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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>

#include "simplefs.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * Free file structure and delete directory entry
 */
void fs_free_file(node_t *file) {
    /* Free: file content, name, node struct
     * Remove: entry in parent hashtable */
    free(file->payload.content);
    hashtable_remove(file->parent->payload.dirhash, file->name);
    free(file->name);
    free(file);
}

/**
 * Free directory structure and delete parent directory entry
 */
int fs_free_dir(node_t *dir) {
    /* Check if dir is empty */
    if (hashtable_get_size(dir->payload.dirhash) == 0) {
        /* Free: hashtable struct, name, node struct
         * Remove: entry in parent hashtable */
        hashtable_destroy(dir->payload.dirhash);
        hashtable_remove(dir->parent->payload.dirhash, dir->name);
        free(dir->name);
        free(dir);
        return 0;
    }
    return -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * Create and return a new string with node full path
 */
char *fs_get_path(node_t *node) {
    char *old, *path;
    /* Climb down from the desired node to the root,
     * adding every node name to the beginning of the path string */
    path = calloc_or_die(1, sizeof(char));
    do {
        old = path;
        path = calloc_or_die((strlen(path) + strlen(node->name) + 2), sizeof(char));
        path[0] = '/';
        strcat(path, node->name);
        strcat(path, old);
        free(old);
        node = node->parent;
    } while (node->parent != NULL);
    return path;
}

/**
 * Get node type, Dir or File
 */
uint8_t fs_get_type(node_t *node) {
    return node->type;
}

/**
 * Get file content
 */
char *fs_get_file_content(node_t *node) {
    if (node->type != File) {
        /* This isn't a file */
        return NULL;
    }
    return node->payload.content;
}

/**
 * Assign new content to a file
 * Return 0 if succeeded, -1 if failed
 */
int fs_set_file_content(node_t *node, char *new_content) {
    if (fs_get_type(node) != File) {
        /* This isn't a file */
        return -1;
    }
    /* Free the old content */
    free(node->payload.content);
    /* Duplicate the new content */
    node->payload.content = my_strdup(new_content);
    return 0;
}

/**
 * Get a node by name from a specific directory, return NULL if not found
 */
node_t *fs_find_in_dir(node_t *parent, char *key) {
    /* Get node from dir hashtable */
    return hashtable_get(parent->payload.dirhash, key);
}

/**
 * Create new empty file or dir in a specific directory
 * Return 0 if succeeded, -1 if failed
 */
int fs_create(node_t *parent, char *key, uint8_t type) {
    if (hashtable_get(parent->payload.dirhash, key) != NULL /* Node already exists */
        || hashtable_get_size(parent->payload.dirhash) >= MAX_NODES /* Dir is full */
        || strlen(key) > MAX_NAMELENGHT /* Name is too long */
        || parent->depth >= MAX_DEPTH) /* Parent node is at max depth */
        return -1;
    /* Create a new empty resource */
    node_t *child = malloc_or_die(sizeof(node_t));
    child->name = my_strdup(key);
    child->depth = parent->depth + 1;
    child->parent = parent;
    child->type = type;
    if (type == Dir) {
        // Empty DirHash
        child->payload.dirhash = hashtable_create();
    } else {
        // Empty content
        child->payload.content = calloc_or_die(1, sizeof(char));
    }
    /* Add resource to its parent's hashtable */
    hashtable_set(parent->payload.dirhash, child->name, child);
    return 0;
}

/**
 * Delete a leaf
 * Return 0 if succeeded, -1 if failed
 */
int fs_delete(node_t *node) {
    if (node->type == Dir) {
        return fs_free_dir(node);
    } else {
        fs_free_file(node);
        return 0;
    }
}

/**
 * Delete a resource recursively
 */
void fs_delete_r(node_t *node) {
    if (node->type == Dir) {
        /* If dir is not empty, delete every child */
        if (hashtable_get_size(node->payload.dirhash) > 0) {
            /* Iterate through the table */
            int state = 0;
            node_t *child = hashtable_iterate(node->payload.dirhash, &state);
            while (child) {
                fs_delete_r(child);
                /* We need to reset the iterator state after
                 * every call to fs_delete_r, as it can change
                 * the order of the elements in the table*/
                state = 0;
                child = hashtable_iterate(node->payload.dirhash, &state);
            }
        }
        fs_free_dir(node);
    } else {
        fs_free_file(node);
    }
}

/**
 * Create a new root directory
 */
node_t *fs_new_root(void) {
    node_t *root;
    root = malloc_or_die(sizeof(node_t));
    root->name = calloc_or_die(1, sizeof(char));
    root->depth = 1;
    root->parent = NULL;
    root->type = Dir;
    root->payload.dirhash = hashtable_create();
    return root;
}

/**
 * Find resources recursively given a starting directory
 */
node_t **fs_find_r(node_t *node, char *name, size_t *num, node_t **array) {
    int state = 0; // Iterator state
    node_t *child = hashtable_iterate(node->payload.dirhash, &state);
    while (child) {
        if (strcmp(child->name, name) == 0) {
            /* We found a node with the requested name */
            *num = *num + 1;
            array = (array == NULL) ? malloc_or_die(sizeof(node_t *))
                                    : realloc_or_die(array, (*num) * sizeof(node_t *));
            array[*num - 1] = child;
        }
        /* Check subdirs */
        if (child->type == Dir) {
            array = fs_find_r(child, name, num, array);
        }
        child = hashtable_iterate(node->payload.dirhash, &state);
    }
    return array;
}