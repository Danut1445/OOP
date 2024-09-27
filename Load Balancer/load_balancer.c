/* Copyright 2023 <> */
// Copyright 2022-2023 Tunsoiu Dan-Andrei 315CA
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "utils.h"
#include "server.h"

// structura server o sa reprezinte replica unui server aceasta
// va retine id-ul serverului din care face parte, hashul replici
// numarul replici si memoria serverului
typedef struct server server;
struct server {
    int server_id;
    unsigned int server_hash;
    int server_part;
    server_memory *server_memory;
};

// load_balancer va contine un vector de pointeri de tipul server
// numarul actual de servere si numarul maxim de servere pe care
// poate sa il stocheze
struct load_balancer {
    server **servers;
    int maxsize;
    int size;
};

unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *)a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

// Cand o sa initializam load_balancerul o sa presupunem ca
// numarul maxim de replici deservere pe care poate sa il
// stocheze este 10
load_balancer *init_load_balancer() {
    load_balancer *main1 = malloc(sizeof(load_balancer));
    DIE(main1 == NULL, "load_balancer malloc");
    main1->maxsize = 10;
    main1->size = 0;
    main1->servers = malloc(main1->maxsize * sizeof(server *));
    DIE(main1->servers == NULL, "load_balancer_servers malloc");
    return main1;
}

void loader_add_server(load_balancer *main, int server_id) {
    server *new, **servers;
    server_memory *server_memory;
    server **aux;
    unsigned int server_hash;
    int st, dr, mij, poz;
    int  server_eti;
    // eset important sa initializam memoria serverului inainte de
    // a creea replicile acestuia pentru ca toate 3 replicile o sa aiba
    // aceeasi memorie de server
    server_memory = init_server_memory();
    for (int i = 0; i < 3; i++) {
        // cream replica unui server
        new = malloc(sizeof(server));
        DIE(new == NULL, "new_server malloc");
        server_eti = i * 100000 + server_id;
        server_hash = hash_function_servers(&server_eti);
        new->server_id = server_id;
        new->server_part = i;
        new->server_hash = server_hash;
        new->server_memory = server_memory;
        st = 0;
        // Pentru ca vectorul de replici este sortat dupa hash atunci o sa
        // folosim cautarea binara pentru a gasi locul in care trebuie pusa
        // noua replica
        if (main->size)
            dr = main->size - 1;
        else
            dr = 0;
        while (st < dr) {
            mij = (st + dr) / 2;
            if (main->servers[mij]->server_hash == server_hash)
                break;
            if (main->servers[mij]->server_hash > server_hash)
                dr = mij - 1;
            else
                st = mij + 1;
        }
        mij = (st + dr) / 2;
        if (main->size) {
            if (main->servers[mij]->server_hash <= server_hash)
                mij = mij + 1;
        }
        main->size++;
        // Daca numarul de replici depaseste numarul maxim de replici pe
        // care il poate retine load_balancerul atunci o sa il reallocam
        // cu o marime de 2 ori mai mare
        if (main->size > main->maxsize) {
            main->maxsize = main->maxsize * 2;
            aux = malloc(main->maxsize * sizeof(server *));
            DIE(aux == NULL, "aux malloc");
            for (int j = 0; j < main->size - 1; j++)
                aux[j] = main->servers[j];
            free(main->servers);
            main->servers = aux;
        }
        for (int j = main->size - 1; j > mij; j--)
            main->servers[j] = main->servers[j - 1];
        main->servers[mij] = new;
        poz = mij;
        servers = main->servers;
        // Dupa aceea verificam daca trebuie mutate obiecte de pe celalalte
        // servere pe noul server adaugat
        if (main->size > 3) {
            if (poz != main->size - 1 && poz != 0) {
                if (servers[poz]->server_id != servers[poz + 1]->server_id)
                    server_move(servers[poz + 1]->server_memory,
                                servers[poz]->server_memory,
                                servers[poz - 1]->server_hash,
                                servers[poz]->server_hash);
            }
            if (poz == main->size - 1) {
                if (servers[poz]->server_id != servers[0]->server_id)
                    server_move(servers[0]->server_memory,
                                servers[poz]->server_memory,
                                servers[poz - 1]->server_hash,
                                servers[poz]->server_hash);
            }
            if (poz == 0) {
                if (servers[poz]->server_id != servers[poz + 1]->server_id)
                    server_move_first(servers[poz + 1]->server_memory,
                                      servers[0]->server_memory,
                                      servers[main->size - 1]->server_hash,
                                      servers[0]->server_hash);
            }
        }
    }
}

void loader_remove_server(load_balancer *main, int server_id) {
    server **aux, **servers;
    unsigned int server_hash;
    int st, dr, mij, poz;
    int  server_eti;
    for (int i = 0; i < 3; i++) {
        server_eti = i * 100000 + server_id;
        server_hash = hash_function_servers(&server_eti);
        st = 0;
        if (main->size)
            dr = main->size - 1;
        else
            dr = 0;
        poz = -1;
        // Asemanator cu cautare gasim replica care trebuie scoase cu
        // ajutoruk cautarii binare
        while (st < dr) {
            mij = (st + dr) / 2;
            if (main->servers[mij]->server_hash == server_hash) {
                poz = mij;
                break;
            }
            if (main->servers[mij]->server_hash > server_hash)
                dr = mij - 1;
            else
                st = mij + 1;
        }
        if (poz == -1) {
            mij = (st + dr) / 2;
            if (main->servers[mij]->server_hash == server_hash)
                poz = mij;
        }
        if (poz == -1) {
            printf("The server given dosent exist\n");
            return;
        }
        servers = main->servers;
        // Apoi o sa mutam obiectele prezente pe replica respectiva pe
        // o replica vecina
        if (poz != main->size - 1 && poz != 0)
            if (servers[poz]->server_id != servers[poz + 1]->server_id)
                server_move(servers[poz]->server_memory,
                            servers[poz + 1]->server_memory,
                            servers[poz - 1]->server_hash,
                            servers[poz + 1]->server_hash);
        if (poz == main->size - 1)
            if (servers[poz]->server_id != servers[0]->server_id)
                server_move(servers[poz]->server_memory,
                            servers[0]->server_memory,
                            servers[poz - 1]->server_hash,
                            servers[poz]->server_hash);
        if (poz == 0)
            if (servers[poz]->server_id != servers[poz + 1]->server_id)
                server_move_first(servers[poz]->server_memory,
                                  servers[poz + 1]->server_memory,
                                  servers[main->size - 1]->server_hash,
                                  servers[poz]->server_hash);
        // Daca este ultima replica a serverului atunci putem elibera memoria
        // ocupata de acesta
        if (i == 2)
            free_server_memory(main->servers[poz]->server_memory);
        free(main->servers[poz]);
        for (int j = poz; j < main->size - 1; j++)
            main->servers[j] = main->servers[j + 1];
        main->size--;
        // Daca load_balancerul contine un numar mai mic de replici decat
        // jumatate din numarul maxim de replici pe care il poate stoca atunci
        // o sa realocam memoria la jumate din cat este acum
        if (main->size <= main->maxsize / 2) {
            main->maxsize = main->maxsize / 2;
            aux = malloc(main->maxsize * sizeof(server *));
            DIE(aux == NULL, "aux malloc");
            for (int j = 0; j < main->size; j++)
                aux[j] = main->servers[j];
            free(main->servers);
            main->servers = aux;
        }
    }
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
    unsigned int key_hash = hash_function_key(key);
    int st, dr, mij, poz = -1;
    st = 0;
    dr = main->size - 1;
    // Cautam serverul pe care trebuie sa adaugam obiectul
    while (st < dr) {
        mij = (st + dr) / 2;
        if (main->servers[mij]->server_hash == key_hash) {
            poz = mij;
            break;
        }
        if (main->servers[mij]->server_hash > key_hash)
            dr = mij - 1;
        else
            st = mij + 1;
    }
    mij = (st + dr) / 2;
    if (main->servers[mij]->server_hash > key_hash) {
        poz = mij;
    } else {
        if (mij + 1 < main->size)
            poz = mij + 1;
        else
            poz = 0;
    }
    *server_id = main->servers[poz]->server_id;
    // Si apoi apela functia de stocare
    server_store(main->servers[poz]->server_memory, key, value);
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
    unsigned int key_hash = hash_function_key(key);
    int st, dr, mij, poz = -1;
    st = 0;
    dr = main->size - 1;
    while (st < dr) {
        mij = (st + dr) / 2;
        if (main->servers[mij]->server_hash == key_hash) {
            poz = mij;
            break;
        }
        if (main->servers[mij]->server_hash > key_hash)
            dr = mij - 1;
        else
            st = mij + 1;
    }
    mij = (st + dr) / 2;
    if (main->servers[mij]->server_hash > key_hash) {
        poz = mij;
    } else {
        if (mij + 1 < main->size)
            poz = mij + 1;
        else
            poz = 0;
    }
    *server_id = main->servers[poz]->server_id;
    return server_retrieve(main->servers[poz]->server_memory, key);
}

void free_load_balancer(load_balancer *main) {
    for (int i = 0; i < main->size; i++) {
        // Daca este prima replica a serverului atunci o sa eliberam
        // memria acestuia
        if (main->servers[i]->server_part == 0)
            free_server_memory(main->servers[i]->server_memory);
        free(main->servers[i]);
    }
    free(main->servers);
    free(main);
}
