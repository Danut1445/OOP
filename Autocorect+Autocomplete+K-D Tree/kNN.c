#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);					\
		}							\
	} while (0)

//Structura pentru nodurile ABC-urilor
typedef struct node node;
struct node {
	node *left;
	node *right;
	int k;
	int *coor;
};

//Structura pentru ABC
typedef struct tree tree;
struct tree {
	node *root;
	int k;
};

//Structura pentru a retine numarul de puncte
//cerute la RS si la NN
typedef struct points points;
struct points {
	int nrmax;
	int nr;
	int **coor;
};

//Functie pentru initializarea ABC-ului
tree *init_bst(int k)
{
	tree *bst;
	bst = malloc(sizeof(tree));
	DIE(!bst, "Error bst malloc");
	bst->k = k;
	bst->root = NULL;
	return bst;
}

//Functie pentru eliberarea din memorie
//a ABC-ului
void free_bst(node *nod)
{
	if (nod->left)
		free_bst(nod->left);
	if (nod->right)
		free_bst(nod->right);
	free(nod->coor);
	free(nod);
}

//Functie care creeaza un nod cu coordonatele date
node *init_node(int k, int *coor)
{
	node *nod = malloc(sizeof(node));
	DIE(!nod, "Error malloc node");
	nod->coor = malloc(k * sizeof(int));
	DIE(!nod->coor, "Error malloc coor");
	nod->k = k;
	for (int i = 0; i < k; i++)
		nod->coor[i] = coor[i];
	nod->left = NULL;
	nod->right = NULL;
	return nod;
}

//Functie care o sa adauge un nou nod cu coordonatele
//date in ABC
void add_node(int *coor, tree *bst)
{
	node *curr = bst->root;
	int lvl = 0, k = bst->k;
	if (!curr) {
		bst->root = init_node(k, coor);
		return;
	}
	while (1) {
		if (coor[lvl % k] < curr->coor[lvl % k]) {
			if (curr->left) {
				curr = curr->left;
				lvl++;
				continue;
			} else {
				curr->left = init_node(k, coor);
				return;
			}
		} else {
			if (curr->right) {
				curr = curr->right;
				lvl++;
				continue;
			} else {
				curr->right = init_node(k, coor);
				return;
			}
		}
	}
}

//Functie pentru a realoca structura de tip point cand aceasta
//nu mia are loc pentru adaugarea unui nou punct
void realloc_point(points *point, int n, int k)
{
	if (n) {
		int **aux;
		aux = malloc(sizeof(int *) * point->nrmax);
		DIE(!aux, "Error alloc aux");
		for (int i = 0; i < point->nrmax; i++)
			aux[i] = point->coor[i];
		free(point->coor);
		point->coor = malloc(2 * point->nrmax * sizeof(int *));
		for (int i = 0; i < point->nrmax; i++)
			point->coor[i] = aux[i];
		for (int i = point->nrmax; i < 2 * point->nrmax; i++) {
			point->coor[i] = malloc(sizeof(int) * k);
			for (int j = 0; j < k; j++)
				point->coor[i][j] = 0;
		}
		free(aux);
		point->nrmax = point->nrmax * 2;
	} else {
		for (int i = 0; i < point->nrmax; i++)
			free(point->coor[i]);
		free(point->coor);
		point->nrmax = 2;
		point->nr = 1;
		point->coor = malloc(sizeof(int *) * point->nrmax);
		for (int i = 0; i < point->nrmax; i++) {
			point->coor[i] = malloc(sizeof(int) * k);
			for (int j = 0; j < k; j++)
				point->coor[i][j] = 0;
		}
	}
}

//Functie care aplica algoritmul de Nearest_neighbour pentru punctul cu
//coordonatele date
void near_neigh(node *nod, points *point, float *dist, int lvl, int *coor)
{
	int n = -1;
	if (coor[lvl % nod->k] < nod->coor[lvl % nod->k] && nod->left) {
		n = 0;
		near_neigh(nod->left, point, dist, lvl + 1, coor);
	} else if (nod->right) {
		n = 1;
		near_neigh(nod->right, point, dist, lvl + 1, coor);
	}
	float distst = 0, distdr = 0, distnod = 0;
	//O sa calculam distamta catre nodul in care ne aflam actual
	//si catre celelalte doua parti ale acestuia pe care le-am notat
	//cu disdr si disst
	for (int i = 0; i < nod->k; i++) {
		if (nod->left) {
			if (coor[i] > nod->coor[i] && lvl % nod->k == i)
				distst = distst + ((coor[i] - nod->coor[i]) *
								   (coor[i] - nod->coor[i]));
		}
		if (nod->right) {
			if (coor[i] < nod->coor[i] && lvl % nod->k == i)
				distdr = distdr + ((coor[i] - nod->coor[i]) *
								   (coor[i] - nod->coor[i]));
		}
		distnod = distnod + (coor[i] - nod->coor[i]) * (coor[i] - nod->coor[i]);
	}
	distnod = sqrt(distnod);
	distdr = sqrt(distdr);
	distst = sqrt(distst);
	if (*dist == -1 || *dist > distnod) {
		realloc_point(point, 0, nod->k);
		for (int i = 0; i < nod->k; i++)
			point->coor[0][i] = nod->coor[i];
		*dist = distnod;
	} else if (*dist == distnod) {
		point->nr++;
		if (point->nr > point->nrmax)
			realloc_point(point, 1, nod->k);
		for (int i = 0; i < nod->k; i++)
			point->coor[point->nr - 1][i] = nod->coor[i];
		*dist = distnod;
	}
	if (n != 0 && (distst <= *dist || *dist == -1) && nod->left)
		near_neigh(nod->left, point, dist, lvl + 1, coor);
	if (n != 1 && (distdr <= *dist || *dist == -1) && nod->right)
		near_neigh(nod->right, point, dist, lvl + 1, coor);
}

//Functia de parcurgere pentru RS care o sa fie un dfs
void dfs(node *nod, points *point, int *coor, int lvl)
{
	int n = lvl % nod->k;
	if (nod->left && coor[2 * n] < nod->coor[n])
		dfs(nod->left, point, coor, lvl + 1);
	if (nod->right && coor[2 * n + 1] >= nod->coor[n])
		dfs(nod->right, point, coor, lvl + 1);
	if (point->nr + 1 > point->nrmax)
		realloc_point(point, 1, nod->k);
	for (int i = 0; i < nod->k; i++) {
		if (nod->coor[i] < coor[2 * i] || nod->coor[i] > coor[2 * i + 1])
			return;
		point->coor[point->nr][i] = nod->coor[i];
	}
	point->nr++;
}

//Functie pentru a sorta vectorul de puncte din interiorul
//structurii de points
void bubble_sort_point(points *point, int k)
{
	int ok = 0, *aux;
	do {
		ok = 0;
		for (int i = 0; i < (point->nr) - 1; i++) {
			for (int j = 0; j < k; j++) {
				if (point->coor[i][j] < point->coor[i + 1][j]) {
					break;
				} else if (point->coor[i][j] > point->coor[i + 1][j]) {
					aux = point->coor[i];
					point->coor[i] = point->coor[i + 1];
					point->coor[i + 1] = aux;
					ok = 1;
					break;
				}
			}
		}
	} while (ok);
}

//Fucntie pentru alocarea unei structuri de tipul points
points *point_malloc(tree *bst)
{
	points *point = malloc(sizeof(points));
	DIE(!point, "Error malloc point");
	point->nrmax = 2;
	point->nr = 0;
	point->coor = malloc(sizeof(int *) * point->nrmax);
	DIE(!point->coor, "Error alloc point->coor");
	for (int i = 0; i < point->nrmax; i++) {
		point->coor[i] = malloc(sizeof(int) * bst->k);
		DIE(!point->coor[i], "Error alloc point->coor[i]");
		for (int j = 0; j < bst->k; j++)
			point->coor[i][j] = 0;
	}
	return point;
}

//Functia main care va contine citirea comenzilor de la
//tastatura si apelarea functilor necesare in functie de acestea
int main(void)
{
	tree *bst;
	char command[256], file_name[256];
	FILE *f;
	int n, k, *coor, aux;
	float dist = -1;
	points *point;
	while (1) {
		scanf("%s", command);
		if (!strcmp(command, "LOAD")) {
			scanf("%s", file_name);
			f = fopen(file_name, "r");
			DIE(!f, "File dosent exitst");
			fscanf(f, "%d%d", &n, &k);
			bst = init_bst(k);
			coor = malloc(sizeof(int) * k);
			DIE(!coor, "coor malloc error");
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < k; j++) {
					fscanf(f, "%d", &aux);
					coor[j] = aux;
				}
				add_node(coor, bst);
			}
			free(coor);
			fclose(f);
			//printfpreord(bst->root);
		}
		if (!strcmp(command, "NN")) {
			dist = -1;
			coor = malloc(sizeof(int) * k);
			DIE(!coor, "coor malloc error");
			for (int i = 0; i < k; i++) {
				scanf("%d", &aux);
				coor[i] = aux;
			}
			point = point_malloc(bst);
			near_neigh(bst->root, point, &dist, 0, coor);
			bubble_sort_point(point, bst->k);
			for (int i = 0; i < point->nr; i++) {
				for (int j = 0; j < bst->k; j++)
					printf("%d ", point->coor[i][j]);
				printf("\n");
			}
			for (int i = 0; i < point->nrmax; i++)
				free(point->coor[i]);
			free(point->coor);
			free(point);
			free(coor);
		}
		if (!strcmp(command, "RS")) {
			coor = malloc(sizeof(int) * k * 2);
			DIE(!coor, "coor malloc error");
			for (int i = 0; i < 2 * bst->k; i = i + 2) {
				scanf("%d", &aux);
				coor[i] = aux;
				scanf("%d", &aux);
				coor[i + 1] = aux;
			}
			point = point_malloc(bst);
			dfs(bst->root, point, coor, 0);
			bubble_sort_point(point, bst->k);
			for (int i = 0; i < point->nr; i++) {
				for (int j = 0; j < bst->k; j++)
					printf("%d ", point->coor[i][j]);
				printf("\n");
			}
			for (int i = 0; i < point->nrmax; i++)
				free(point->coor[i]);
			free(point->coor);
			free(point);
			free(coor);
		}
		if (!strcmp(command, "EXIT")) {
			if (bst->root)
				free_bst(bst->root);
			free(bst);
			break;
		}
	}
}
