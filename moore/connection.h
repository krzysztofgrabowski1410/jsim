#ifndef AUTOMATYMOORE_A_CONNECTION_H
#define AUTOMATYMOORE_A_CONNECTION_H

#include "ma.h"

struct conn_d;

//structure representing a single node
//in a list of connections between two machines, located in source
typedef struct conn_s {
    moore_t* other;
    size_t start_source;
    size_t start_destination;
    size_t length;
    struct conn_d* corresponding_node;
    //variables responsible for keeping it as a list:
    struct conn_s* left;
    struct conn_s* right;
} conn_s_t;

//structure representing a single node
//in a list of connections between two machines, located in destination
typedef struct conn_d {
    size_t start;
    size_t length;
    conn_s_t* corresponding_node;
    //variables responsible for keeping it as a list:
    struct conn_d* left;
    struct conn_d* right;
} conn_d_t;

//a list of conn_d
typedef conn_d_t* list_d_t;

//a list of conn_s
typedef conn_s_t* list_s_t;

void set_to_0_d(conn_d_t* node);
void set_to_0_s(conn_s_t* node);

void pop_s(conn_s_t* node);
void pop_d(conn_d_t* node);

int build_empty(list_d_t list_d, size_t size_d, list_s_t list_s, size_t size_s);

conn_d_t* initiate_list_d();

void push_d(list_d_t list, conn_d_t* node);

void clear_list_d(list_d_t list);

conn_s_t* initiate_list_s();

void push_s(list_s_t list, conn_s_t* node);

void clear_list_s(list_s_t list);

void send_info_connection(moore_t* machine);

#endif //AUTOMATYMOORE_A_CONNECTION_H
