#include <stdlib.h>
#include "connection.h"


void set_to_0_d(conn_d_t* node) {
    node->start = 0;
    node->length = 0;
    node->corresponding_node = NULL;
}

void set_to_0_s(conn_s_t* node) {
    node->other = NULL;
    node->start_source = 0;
    node->start_destination = 0;
    node->length = 0;
    node->corresponding_node = NULL;
}

//assuming that lists are initialized
//return -1 if allocation failed; the list will be cleared in another function
int build_empty(list_d_t list_d, size_t size_d, list_s_t list_s, size_t size_s) {
    size_s += 2; //buffer
    size_d += 2; //buffer

    for (size_t i = 0; i < size_d; i++) {
        conn_d_t* curr = (conn_d_t*)malloc(sizeof(conn_d_t));
        if (curr == NULL) {
            return -1;
        }
        set_to_0_d(curr);
        push_d(list_d, curr);
    }

    for (size_t i = 0; i < size_s; i++) {
        conn_s_t* curr = (conn_s_t*)malloc(sizeof(conn_s_t));
        if (curr == NULL) {
            return -1;
        }
        set_to_0_s(curr);
        push_s(list_s, curr);
    }

    return 0;
}

void pop_d(conn_d_t* node) {
    node->left->right = node->right;
    node->right->left = node->left;
    free(node);
}

void pop_s(conn_s_t* node) {
    node->left->right = node->right;
    node->right->left = node->left;
    free(node);
}


conn_d_t* initiate_list_d() {
    conn_d_t* left_guardian = (conn_d_t*)malloc(sizeof(conn_d_t));
    if (left_guardian == NULL) {
        return NULL;
    }
    conn_d_t* right_guardian = (conn_d_t*)malloc(sizeof(conn_d_t));
    if (right_guardian == NULL) {
        free(left_guardian);
        return NULL;
    }

    left_guardian->start = 0;
    left_guardian->length = 0;
    left_guardian->corresponding_node = NULL;
    left_guardian->left = NULL;
    left_guardian->right = right_guardian;

    right_guardian->start = 0;
    right_guardian->length = 0;
    right_guardian->corresponding_node = NULL;
    right_guardian->left = left_guardian;
    right_guardian->right = NULL;

    return left_guardian;
}

conn_s_t* initiate_list_s() {
    conn_s_t* left_guardian = (conn_s_t*)malloc(sizeof(conn_s_t));
    if (left_guardian == NULL) {
        return NULL;
    }
    conn_s_t* right_guardian = (conn_s_t*)malloc(sizeof(conn_s_t));
    if (right_guardian == NULL) {
        free(left_guardian);
        return NULL;
    }

    left_guardian->other = NULL;
    left_guardian->start_source = 0;
    left_guardian->start_destination = 0;
    left_guardian->length = 0;
    left_guardian->corresponding_node = NULL;
    left_guardian->left = NULL;
    left_guardian->right = right_guardian;

    right_guardian->other = NULL;
    right_guardian->start_source = 0;
    right_guardian->start_destination = 0;
    right_guardian->length = 0;
    right_guardian->corresponding_node = NULL;
    right_guardian->left = left_guardian;
    right_guardian->right = NULL;

    return left_guardian;
}

//clears also corresponding nodes
void clear_list_d(list_d_t list) {
    while(list->right != NULL) {
        conn_d_t* next = list->right;
        if (list->corresponding_node != NULL) {
            pop_s(list->corresponding_node);
        }
        free(list);
        list = next;
    }
    free(list);
}

//clears also corresponding nodes
void clear_list_s(list_s_t list) {
    while (list->right != NULL) {
        conn_s_t* next = list->right;
        if (list->corresponding_node != NULL) {
            pop_d(list->corresponding_node);
        }
        free(list);
        list = next;
    }
    free(list);
}

void push_d(list_d_t list, conn_d_t* node) {
    node->left = list;
    node->right = list->right;
    list->right->left = node;
    list->right = node;
}

void push_s(list_s_t list, conn_s_t* node) {
    node->left = list;
    node->right = list->right;
    list->right->left = node;
    list->right = node;
}
