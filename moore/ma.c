#include "ma.h"
#include "connection.h"
#include <stdlib.h>
#include <errno.h>

//globally: first bit has id 0

struct moore {
    //current input:
    uint64_t* input;

    size_t n;
    size_t m;
    size_t s;

    transition_function_t t;
    output_function_t y;

    //current state:
    uint64_t* q;
    uint64_t* output;

    //connection arrays:
    list_d_t connection_d;
    list_s_t connection_s;

    //used for false mallocs inside disconnect (and connect) function
    list_d_t source_for_false_mallocs_d;
    list_s_t source_for_false_mallocs_s;
};

void pop_no_free_d(conn_d_t* node) {
    node->left->right = node->right;
    node->right->left = node->left;
}

void pop_no_free_s(conn_s_t* node) {
    node->left->right = node->right;
    node->right->left = node->left;
}

conn_d_t* get_first_d(list_d_t list) {
    conn_d_t* output = list->right;
    if (output->right != NULL) {
        output->left->right = output->right;
        output->right->left = output->left;
    } else {
        //this code is supposed to be unreachable,
        //but it is added as a guardian to prevent unwanted behavior
        list->right = NULL;
    }
    return output;
}

conn_s_t* get_first_s(list_s_t list) {
    conn_s_t* output = list->right;
    if (output->right != NULL) {
        output->left->right = output->right;
        output->right->left = output->left;
    } else {
        list->right = NULL;
        //should never be here
    }
    return output;
}

size_t ceiling(uint64_t l, uint64_t d) {
    if (l % d == 0) return l / d;
    return (l / d) + 1;
}

//assuming dest and source both the same size
void copy_uint64_t(uint64_t* dest, const uint64_t* source, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dest[i] = source[i];
    }
}

//copies bits from a single uint64_t to another uint64_t
void copy_bits(uint64_t source, size_t start_source, uint64_t * destination,
               size_t start_destination, size_t length) {
    if (length == 0) return;
    if (start_destination > 63) return;
    if (start_source > 63) return;
    if (start_source + length > 63) return;
    if (start_destination + length > 63) return;

    //clear irrelevant bits from source (a copy of actual source)
    source = source << (64 - start_source - length);
    source = source >> (64 - length);
    source = source << start_destination;

    //create masks
    uint64_t mask1;
    if (start_destination == 0) {
        mask1 = 0;
    } else {
        mask1 = UINT64_MAX << (64 - start_destination);
        mask1 = mask1 >> (64 - start_destination);
    }
    uint64_t mask2;
    if (length + start_destination >= 64) {
        mask2 = 0;
    }
    else {
        mask2 = UINT64_MAX >> (length + start_destination);
        mask2 = mask2 << (length + start_destination);
    }

    //copy the relevant bits
    *destination = *destination & (mask1 | mask2);
    *destination = (*destination) | source;
}

//copies bits from uint64_t array to another uint64_t array
void copy_bits_globally(const uint64_t* source, size_t source_id,
                        uint64_t* destination, size_t destination_id,
                        size_t length) {
    if (length == 0) return;

    size_t i = source_id / 64;
    size_t j = destination_id / 64;

    //remaining bits from current unit64_t
    size_t wz1 = 64 - (source_id % 64);
    size_t wz2 = 64 - (destination_id % 64);

    while (length > 0) {
        if (length <= wz1 && length <= wz2) {
            copy_bits(source[i], source_id - 64*i, &(destination[j]),
                      destination_id - 64*j, length);
            break;
        }

        if (wz1 < wz2) {
            copy_bits(source[i], source_id - 64*i, &(destination[j]),
                      destination_id - 64*j, wz1);
            source_id += wz1;
            destination_id += wz1;
            length -= wz1;
        } else {
            copy_bits(source[i], source_id-64*i, &(destination[j]),
                      destination_id-64*j, wz2);
            source_id += wz2;
            destination_id += wz2;
            length -= wz2;
        }

        i = source_id / 64;
        j = destination_id / 64;
        wz1 = 64 - (source_id % 64);
        wz2 = 64 - (destination_id % 64);
    }
}

//these "false mallocs" are a substitute for a malloc
//used inside of disconnect (and connect) function
//the memory is actually allocated when constructing the machine
conn_d_t* false_malloc_disconnect_d(moore_t* machine) {
    return get_first_d(machine->source_for_false_mallocs_d);
}

conn_s_t* false_malloc_disconnect_s(moore_t* machine) {
    return get_first_s(machine->source_for_false_mallocs_s);
}

//these are "false free" commands "freeing" memory for future "false mallocs"
void false_free_d(moore_t* machine, conn_d_t* node) {
    set_to_0_d(node);
    push_d(machine->source_for_false_mallocs_d, node);
}

void false_free_s(moore_t* machine, conn_s_t* node) {
    set_to_0_s(node);
    push_s(machine->source_for_false_mallocs_s, node);
}

void idd(uint64_t* output, uint64_t const *state,
        size_t m, size_t s) {
    copy_uint64_t(output, state, m);
}

//disconnects from range within a single node of connection structures
//start is the starting bit in the destination of connection
//returns a pointer to a node in the right of given node,
//omitting the nodes that this function creates
conn_d_t *disconnect_range_node(conn_d_t *node, size_t start, size_t length,
                                moore_t* machine) {
    conn_d_t* output = node->right;

    //if it is out of range, then just skips the node
    if (start > node->start + node->length - 1) return output;
    if (start + length - 1 < node->start) return output;

    conn_s_t* node_s = node->corresponding_node;

    //if whole node is deleted
    if (start == node->start && length == node->length) {
        //false free
        pop_no_free_s(node_s);
        pop_no_free_d(node);
        false_free_s(machine, node_s);
        false_free_d(machine, node);
        return output;
    }

    //if left part exists
    if (start > node->start) {

        conn_s_t* left_s = false_malloc_disconnect_s(machine);
        left_s->other = node_s->other;
        left_s->start_source = node_s->start_source;
        left_s->start_destination = node_s->start_destination;
        left_s->length = start - node_s->start_destination;
        left_s->left = node_s->left;
        node_s->left->right = left_s;

        conn_d_t* left = false_malloc_disconnect_d(machine);
        left->start = node->start;
        left->length = start - node->start;
        left->left = node->left;
        node->left->right = left;

        left->corresponding_node = left_s;
        left_s->corresponding_node = left;

        //if right part doesn't exist (and left part does)
        if (start + length >= node->start + node->length) {

            left->right = node->right;
            node->right->left = left;
            left_s->right = node_s->right;
            node_s->right->left = left_s;

            false_free_s(machine, node_s);
            false_free_d(machine, node);
            return output;
        }
    }

    //if right part exists
    if (start + length < node->start + node->length) {

        conn_d_t* right = false_malloc_disconnect_d(machine);
        right->start = start + length;
        right->length = node->start + node->length - right->start;
        right->right = node->right;
        node->right->left = right;

        conn_s_t* right_s = false_malloc_disconnect_s(machine);
        right_s->other = node_s->other;
        right_s->start_source = node_s->start_source +
                (start + length - node_s->start_destination);
        right_s->start_destination = start + length;
        right_s->length = right->length;
        right_s->right = node_s->right;
        node_s->right->left = right_s;

        right->corresponding_node = right_s;
        right_s->corresponding_node = right;

        //if left part doesn't exist
        if (start <= node->start) {

            right->left = node->left;
            node->left->right = right;

            right_s->left = node_s->left;
            node_s->left->right = right_s;


            false_free_s(machine, node_s);
            false_free_d(machine, node);
            return output;
        }
    }

    //if both parts exist:

    //The same as: left_s->right = right_s;
    node_s->left->right->right = node_s->right->left;

    //The same as: right_s->left = left_s;
    node_s->right->left->left = node_s->left->right;

    false_free_s(machine, node_s);

    //The same as: left->right = right;
    node->left->right->right = node->right->left;
    //The same as: right->left = left;
    node->right->left->left = node->left->right;

    false_free_d(machine, node);
    return output;
}

//start is the starting bit in the destination of connection
void disconnect_range_list(moore_t* machine, size_t start, size_t length) {
    conn_d_t* current = machine->connection_d->right;
    while (current->right != NULL) {
        if (start < current->start + current->length &&
            start + length >= current->start) {

            current = disconnect_range_node(current, start, length, machine);
        }
    }
}

//sends info about changed output to every machine that need that information
void send_info_connection(moore_t* machine) {
    conn_s_t* curr = machine->connection_s->right;
    while (curr->right != NULL) {
        copy_bits_globally(machine->output, curr->start_source,
                           curr->other->input, curr->start_destination,
                           curr->length);
        curr = curr->right;
    }
}

moore_t * ma_create_full(size_t n, size_t m, size_t s, transition_function_t t,
                         output_function_t y, uint64_t const *q) {

    if (m == 0 || s == 0 || t == NULL || y == NULL || q == NULL) {
        errno = EINVAL;
        return NULL;
    }
    moore_t* machine = (moore_t*)malloc(sizeof(moore_t));
    if (machine == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    machine->q = (uint64_t*)malloc(ceiling(s, 64)*sizeof(uint64_t));
    machine->output = (uint64_t*)malloc(ceiling(m, 64)*sizeof(uint64_t));
    if (machine->q == NULL || machine->output == NULL) {
        errno = ENOMEM;
        free(machine->output);
        free(machine->q);
        free(machine);
        return NULL;
    }

    machine->input = NULL;
    if (n > 0) {
        machine->input = (uint64_t*)malloc(ceiling(n, 64) * sizeof(uint64_t));
    }
    machine->n = n;
    machine->m = m;
    machine->s = m;
    machine->t = t;
    machine->y = y;
    copy_uint64_t(machine->q, q, ceiling(s, 64)); //setting q
    //setting output
    machine->y(machine->output, machine->q, machine->m, machine->s);
    machine->connection_d = initiate_list_d();
    machine->connection_s = initiate_list_s();
    machine->source_for_false_mallocs_d = initiate_list_d();
    machine->source_for_false_mallocs_s = initiate_list_s();

    int err1 = -1; //error while allocating source for false mallocs
    if(machine->source_for_false_mallocs_d != NULL &&
       machine->source_for_false_mallocs_s != NULL) {

        //if all succeeds, then err1 will be set to 0
        err1 = build_empty(machine->source_for_false_mallocs_d, n,
                           machine->source_for_false_mallocs_s, n);
    }
    if (machine->connection_d == NULL || machine->connection_s == NULL ||
            (machine->input == NULL && n > 0) || err1 == -1) {

        errno = ENOMEM;
        clear_list_s(machine->connection_s);
        clear_list_d(machine->connection_d);
        free(machine->input);
        free(machine->output);
        free(machine->q);
        clear_list_s(machine->source_for_false_mallocs_s);
        clear_list_d(machine->source_for_false_mallocs_d);
        free(machine);
        return NULL;

    }

    //set base input as 0 (however, it is considered unknown by the user)
    for (size_t i = 0; i < ceiling(n, 64); i++) {
        machine->input[i] = 0;
    }

    return machine;
}

moore_t * ma_create_simple(size_t n, size_t m, transition_function_t t) {
    if (m == 0 || t == NULL) {
        errno = EINVAL;
        return NULL;
    }
    uint64_t* q0 = (uint64_t*)malloc(ceiling(m, 64) * sizeof(uint64_t));
    if (q0 == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    for (size_t i = 0; i < ceiling(m, 64); i++) {
        q0[i] = 0;
    }
    moore_t* machine = ma_create_full(n, m, m, t, idd, q0);
    free(q0);
    return machine;
}

void ma_delete(moore_t *a) {
    if (a == NULL) return;
    clear_list_s(a->connection_s);
    clear_list_d(a->connection_d);
    free(a->output);
    free(a->input);
    free(a->q);
    clear_list_s(a->source_for_false_mallocs_s);
    clear_list_d(a->source_for_false_mallocs_d);
    free(a);
}

int ma_connect(moore_t *a_in, size_t in, moore_t *a_out, size_t out,
               size_t num) {
    if (a_in == NULL || a_out == NULL || num == 0) {
        errno = EINVAL;
        return -1;
    }
    if (in + num - 1 >= a_in->n || out + num - 1 >= a_out->m) {
        errno = EINVAL;
        return -1;
    }

    //these mallocs CANNOT FAIL... Well, they are not really mallocs...
    conn_s_t* node_s = false_malloc_disconnect_s(a_out);
    conn_d_t* node_d = false_malloc_disconnect_d(a_out);

    node_d->start = in;
    node_d->length = num;
    node_d->corresponding_node = node_s;
    node_s->other = a_in;
    node_s->start_source = out;
    node_s->start_destination = in;
    node_s->length = num;
    node_s->corresponding_node = node_d;

    disconnect_range_list(a_in, in, num);
    push_d(a_in->connection_d, node_d);
    push_s(a_out->connection_s, node_s);
    copy_bits_globally(a_out->output, out, a_in->input, in,  num);
    return 0;
}

int ma_disconnect(moore_t *a_in, size_t in, size_t num) {
    if (a_in == NULL || num == 0 || in + num - 1 >= a_in->n) {
        errno = EINVAL;
        return -1;
    }
    disconnect_range_list(a_in, in, num);
    return 0;
}

int ma_set_input(moore_t *a, uint64_t const *input) {
    if (a == NULL || a->n == 0 || input == NULL) {
        errno = EINVAL;
        return -1;
    }
    copy_uint64_t(a->input, input, ceiling(a->n, 64));
    return 0;
}

int ma_set_state(moore_t *a, uint64_t const *state) {
    if (a == NULL || state == NULL) {
        errno = EINVAL;
        return -1;
    }
    copy_uint64_t(a->q, state, ceiling(a->s, 64));
    a->y(a->output, a->q, a->m, a->s);
    send_info_connection(a);
    return 0;
}

uint64_t const * ma_get_output(moore_t const *a) {
    if (a == NULL) {
        errno = EINVAL;
        return NULL;
    }
    return a->output;
}

int ma_step(moore_t *at[], size_t num) {
    if (at == NULL || num == 0) {
        errno = EINVAL;
        return -1;
    }
    size_t maxS = 1;
    for (size_t i = 0; i < num; i++) {
        if (at[i]->s > maxS) {
            maxS = at[i]->s;
        }
        if (at[i] == NULL) {
            errno = EINVAL;
            return -1;
        }
    }

    uint64_t *tempStateSaver =
            (uint64_t *)malloc(ceiling(maxS, 64) * sizeof(uint64_t));
    if (tempStateSaver == NULL) {
        errno = ENOMEM;
        return -1;
    }

    //calculate new states of each machine
    for (size_t i = 0; i < num; i++) {
        at[i]->t(tempStateSaver, at[i]->input, at[i]->q, at[i]->n, at[i]->s);
        copy_uint64_t(at[i]->q, tempStateSaver, ceiling(at[i]->s, 64));
    }
    free(tempStateSaver);

    //calculate new outputs
    for (size_t i = 0; i < num; i++) {
        at[i]->y(at[i]->output, at[i]->q, at[i]-> m, at[i]->s);
    }

    //send information to receivers of updated outputs
    for (size_t i = 0; i < num; i++) {
        send_info_connection(at[i]);
    }
    return 0;
}
