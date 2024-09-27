/* Copyright 2023 <> */
// Copyright 2022-2023 Tunsoiu Dan-Andrei 315CA
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "load_balancer.h"
#include "utils.h"

#define KEY_LENGTH 128
#define VALUE_LENGTH 65536

typedef struct ll_node_t
{
    void* data;
    struct ll_node_t* next;
} ll_node_t;

typedef struct linked_list_t
{
    ll_node_t* head;
    unsigned int data_size;
    unsigned int size;
} linked_list_t;

typedef struct produs produs;
struct produs{
    char key[KEY_LENGTH];
    char value[VALUE_LENGTH];
    unsigned int hash_key;
};

struct server_memory {
    unsigned int size;
    linked_list_t** listaprodus;
};

linked_list_t *
ll_create(unsigned int data_size)
{
    linked_list_t* ll;

    ll = malloc(sizeof(*ll));
    DIE(ll == NULL, "linked_list malloc");

    ll->head = NULL;
    ll->data_size = data_size;
    ll->size = 0;

    return ll;
}

void
ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data)
{
    ll_node_t *prev, *curr;
    ll_node_t* new_node;

    if (list == NULL) {
        return;
    }

    /* n >= list->size inseamna adaugarea unui nou nod la finalul listei. */
    if (n > list->size) {
        n = list->size;
    }

    curr = list->head;
    prev = NULL;
    while (n > 0) {
        prev = curr;
        curr = curr->next;
        --n;
    }

    new_node = malloc(sizeof(*new_node));
    DIE(new_node == NULL, "new_node malloc");
    new_node->data = malloc(list->data_size);
    DIE(new_node->data == NULL, "new_node->data malloc");
    memcpy(new_node->data, new_data, list->data_size);

    new_node->next = curr;
    if (prev == NULL) {
        /* Adica n == 0. */
        list->head = new_node;
    } else {
        prev->next = new_node;
    }

    list->size++;
}

ll_node_t *
ll_remove_nth_node(linked_list_t* list, unsigned int n)
{
    ll_node_t *prev, *curr;

    if (list == NULL) {
        return NULL;
    }

    if (list->head == NULL) { /* Lista este goala. */
        return NULL;
    }

    /* n >= list->size - 1 inseamna eliminarea nodului de la finalul listei. */
    if (n > list->size - 1) {
        n = list->size - 1;
    }

    curr = list->head;
    prev = NULL;
    while (n > 0) {
        prev = curr;
        curr = curr->next;
        --n;
    }

    if (prev == NULL) {
        /* Adica n == 0. */
        list->head = curr->next;
    } else {
        prev->next = curr->next;
    }

    list->size--;

    return curr;
}

unsigned int
ll_get_size(linked_list_t* list)
{
    if (list == NULL) {
        return -1;
    }

    return list->size;
}

void
ll_free(linked_list_t* pp_list)
{
    ll_node_t* currNode;

    if (pp_list == NULL) {
        return;
    }

    while (ll_get_size(pp_list) > 0) {
        currNode = ll_remove_nth_node(pp_list, 0);
        if (currNode) {
            free(currNode->data);
            currNode->data = NULL;
            free(currNode);
            currNode = NULL;
        } else {
            pp_list->size = 0;
        }
    }

    free(pp_list);
}

// server_memory o sa fie un vector de hashtabeluri pe care o sa le
// initializam in functia de mai jos
server_memory *init_server_memory()
{
    linked_list_t *lista;
    server_memory *server = malloc(sizeof(server_memory));
    DIE(server == NULL, "server malloc");
    server->size = 200;
    server->listaprodus = malloc(200 * sizeof(linked_list_t *));
    DIE(server->listaprodus == NULL, "server_lista_produs malloc");
    for (int i = 0; i < 200; i++) {
        lista = ll_create(sizeof(produs));
        DIE(lista == NULL, "lista malloc");
        server->listaprodus[i] = lista;
    }
    return server;
}

void server_store(server_memory *server, char *key, char *value) {
    unsigned int poz;
    produs *produs = malloc(sizeof(struct produs));
    strcpy(produs->key, key);
    strcpy(produs->value, value);
    produs->hash_key = hash_function_key(key);
    poz = produs->hash_key % server->size;
    ll_add_nth_node(server->listaprodus[poz], 0, produs);
    free(produs);
}

char *server_retrieve(server_memory *server, char *key) {
    unsigned int poz;
    ll_node_t *nod;
    produs *produs;
    poz = hash_function_key(key) % server->size;
    nod = server->listaprodus[poz]->head;
    while (nod) {
        produs = (struct produs *) nod->data;
        if (!strcmp(key, produs->key))
            return produs->value;
        nod = nod->next;
    }
    return NULL;
}

// Functia o sa primeasca ca parametri memoria serverului de pe care
// trebuie mutat obiectul, memoria pe care trebuie mutata si intre ce
// hash trebuie sa se afle hashul obiectului
void server_move(server_memory *server1, server_memory *server2,
                 unsigned int addr1, unsigned int addr2) {
    ll_node_t *nod, *aux;
    produs *produs;
    for (int i = 0; i < 200; i++) {
        nod = server1->listaprodus[i]->head;
        if (nod) {
            // In while o sa verificam daca primul element din lista unui bucket
            // trebuie mutat, cazul in care o sa il mutam si apoi al doilea
            // element din lista o sa devina noul prim element pentru care
            // o sa trebuiasca sa facem iarasi verificarea
            while (1) {
                if (nod)
                    produs = (struct produs *)nod->data;
                else
                    break;
                // Verifica daca elemntul trebuie mutat
                if (produs->hash_key > addr1 && produs->hash_key <= addr2) {
                    aux = nod;
                    server1->listaprodus[i]->head = aux->next;
                    aux->next = server2->listaprodus[i]->head;
                    server2->listaprodus[i]->head = aux;
                    server2->listaprodus[i]->size++;
                    server1->listaprodus[i]->size--;
                    nod = server1->listaprodus[i]->head;
                } else {
                    // Daca primul element nu trebuie mutat atunci o sa iesim
                    // din while
                    break;
                }
            }
            // Daca lista mai are elemente (si nu este primul din lista
            // atunci o sa verificam pentru fiecare daca trebuie mutate
            if (nod) {
                while (nod->next) {
                    produs = (struct produs *)nod->next->data;
                    if (produs->hash_key > addr1 && produs->hash_key <= addr2) {
                        aux = nod->next;
                        nod->next = aux->next;
                        aux->next = server2->listaprodus[i]->head;
                        server2->listaprodus[i]->head = aux;
                        server2->listaprodus[i]->size++;
                        server1->listaprodus[i]->size--;
                    } else {
                    nod = nod->next;
                    }
                }
            }
        }
    }
}

// Aproape identica cu functia anterioara, doar if-urile se schimba pentru
// ca o sa o folosim atunci cand trebuie sa mutam elemntele de pe primul server
// din load balancer care au o relatie diferita fata de restul serverelor
void server_move_first(server_memory *server1, server_memory *server2,
                 unsigned int addr1, unsigned int addr2) {
    ll_node_t *nod, *aux;
    produs *produs;
    for (int i = 0; i < 200; i++) {
        nod = server1->listaprodus[i]->head;
        if (nod) {
            while (1) {
                if (nod)
                    produs = (struct produs *)nod->data;
                else
                    break;
                if (produs->hash_key > addr1 || produs->hash_key <= addr2) {
                    aux = nod;
                    server1->listaprodus[i]->head = aux->next;
                    aux->next = server2->listaprodus[i]->head;
                    server2->listaprodus[i]->head = aux;
                    server2->listaprodus[i]->size++;
                    server1->listaprodus[i]->size--;
                    nod = server1->listaprodus[i]->head;
                } else {
                    break;
                }
            }
            if (nod) {
                while (nod->next) {
                    produs = (struct produs *)nod->next->data;
                    if (produs->hash_key > addr1 || produs->hash_key <= addr2) {
                        aux = nod->next;
                        nod->next = aux->next;
                        aux->next = server2->listaprodus[i]->head;
                        server2->listaprodus[i]->head = aux;
                        server2->listaprodus[i]->size++;
                        server1->listaprodus[i]->size--;
                    } else {
                    nod = nod->next;
                    }
                }
            }
        }
    }
}

void server_remove(server_memory *server, char *key) {
    ll_node_t *nod;
    produs *produs;
    int j;
    for (int i = 0; i < 200; i++) {
        j = 0;
        nod = server->listaprodus[i]->head;
        while (nod) {
            produs = (struct produs *)nod->data;
            if (!strcmp(produs->key, key)) {
                ll_remove_nth_node(server->listaprodus[i], j);
                return;
            }
            j++;
            nod = nod->next;
        }
    }
    printf("Nu sa gasit produsul\n");
}

void free_server_memory(server_memory *server) {
    for (unsigned int i = 0; i < server->size; i++)
        ll_free(server->listaprodus[i]);
    free(server->listaprodus);
    free(server);
}
