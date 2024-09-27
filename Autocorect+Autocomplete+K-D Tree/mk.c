#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define ALPHABET_SIZE 26
#define ALPHABET "abcdefghijklmnopqrstuvwxyz"

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);					\
		}							\
	} while (0)

//Structura pentru nodurile trie-ului
typedef struct trie_node_t trie_node_t;
struct trie_node_t {
	int nrwords;
	trie_node_t **children;
	int n_children;
};

//Structura pentru trie
typedef struct trie_t trie_t;
struct trie_t {
	trie_node_t *root;
	int size;
	int alphabet_size;
	char *alphabet;
};

//Functie de creearea a nodului unui trie
trie_node_t *trie_create_node(trie_t *trie)
{
	trie_node_t *node = malloc(sizeof(trie_node_t));
	DIE(!node, "Error malloc node");
	node->nrwords = 0;
	node->children = malloc(sizeof(trie_node_t *) * trie->alphabet_size);
	DIE(!node->children, "Erroe malloc node->children");
	for (int i = 0; i < ALPHABET_SIZE; i++)
		node->children[i] = NULL;
	node->n_children = 0;
	return node;
}

//Functie de creeare a trieului
trie_t *trie_create(int alphabet_size, char *alphabet)
{
	trie_t *trie = malloc(sizeof(trie_t));
	DIE(!trie, "Error malloc trie");
	trie->size = 0;
	trie->root = NULL;
	trie->alphabet_size = alphabet_size;
	trie->alphabet = alphabet;
	return trie;
}

//Functie de inserarea al unui cuvant in trie
void trie_insert(trie_t *trie, char *key)
{
	int k = 0;
	if (!trie->root)
		trie->root = trie_create_node(trie);
	trie_node_t *curr = trie->root;
	while (key[k]) {
		if (!curr->children[key[k] - 'a']) {
			curr->children[key[k] - 'a'] = trie_create_node(trie);
			curr->n_children++;
		}
		curr = curr->children[key[k] - 'a'];
		k++;
	}
	curr->nrwords++;
}

//Functie recursiva de stergere a nodurilor care apartin unui cuvant
//din trie
int remove_word(trie_t *trie, trie_node_t *curr, char *key, int k)
{
	int n;
	if (key[k]) {
		if (!curr->children[key[k] - 'a'])
			return -1;
		n = remove_word(trie, curr->children[key[k] - 'a'], key, k + 1);
	}
	if (!key[k] && !curr->nrwords)
		return -1;
	if (!key[k]) {
		curr->nrwords = 0;
		return curr->n_children;
	}
	if (!n) {
		free(curr->children[key[k] - 'a']->children);
		free(curr->children[key[k] - 'a']);
		curr->children[key[k] - 'a'] = NULL;
		curr->n_children--;
	}
	if (!curr->nrwords)
		return curr->n_children;
	else
		return -1;
}

//Functie de stergere al unui cuvant din trie
void trie_remove(trie_t *trie, char *key)
{
	int k = 0;
	trie_node_t *curr = trie->root;
	remove_word(trie, curr, key, k);
}

//Functie de eliberare a memoriei unui nod
void free_node(trie_node_t *curr)
{
	for (int i = 0; i < ALPHABET_SIZE; i++)
		if (curr->children[i]) {
			free_node(curr->children[i]);
			free(curr->children[i]);
		}
	free(curr->children);
}

//Functie de stergere a trieului
void trie_free(trie_t **ptrie)
{
	trie_t *trie = *ptrie;
	trie_node_t *curr;
	curr = trie->root;
	if (curr) {
		free_node(curr);
		free(trie->root);
	}
	free(trie);
	ptrie = NULL;
}

//Functie pentru parcurgere dfs in cadrul comenzii de AUTOCORRECT
void dfs_cor(trie_node_t *node, char *word, int k, int dif,
			 char *found, int lvl, int *nr)
{
	if (dif > k)
		return;
	if (lvl > (int)strlen(word)) {
		if (node->nrwords) {
			found[lvl - 1] = 0;
			printf("%s\n", found);
			(*nr)++;
		}
		return;
	}
	for (int i = 0; i < 26; i++)
		if (node->children[i]) {
			found[lvl - 1] = 'a' + i;
			if ('a' + i != word[lvl - 1])
				dfs_cor(node->children[i], word, k, dif + 1,
						found, lvl + 1, nr);
			else
				dfs_cor(node->children[i], word, k, dif,
						found, lvl + 1, nr);
		}
}

//Functia de AUTOCORRECT
void auto_correct(trie_t *trie, char *word, int k)
{
	char found[256];
	int nr = 0;
	dfs_cor(trie->root, word, k, 0, found, 1, &nr);
	if (nr == 0)
		printf("No words found\n");
}

//Functie de parcurgere de tip dfs in cadrul operatiei de AUTOCOMPLETE
void dfs_compl(trie_node_t *node, char *word, int *ok, int *gasit,
			   int *apar, int lvl, int *lung, char *small, char *sho,
			   char *frecv, char *new_s, char *new_f)
{
	if (lvl <= (int)strlen(word)) {
		if (node->children[word[lvl - 1] - 'a']) {
			small[lvl - 1] = word[lvl - 1];
			new_s[lvl - 1] = word[lvl - 1];
			new_f[lvl - 1] = word[lvl - 1];
			if (lvl == (int)strlen(word) &&
				node->children[word[lvl - 1] - 'a']->nrwords != 0) {
				*gasit = 1;
				small[lvl] = 0;
				new_s[lvl] = 0;
				strcpy(sho, new_s);
				new_f[lvl] = 0;
				strcpy(frecv, new_f);
				*apar = node->children[word[lvl - 1] - 'a']->nrwords;
				*lung = (int)strlen(new_s);
			}
			dfs_compl(node->children[word[lvl - 1] - 'a'], word, ok, gasit,
					  apar, lvl + 1, lung, small, sho, frecv, new_s, new_f);
		}
	} else {
		*ok = 1;
		for (int i = 0; i < 26; i++) {
			if (node->children[i]) {
				if (*gasit == 0) {
					small[lvl - 1] = i + 'a';
					if (node->children[i]->nrwords != 0) {
						*gasit = 1;
						small[lvl] = 0;
					}
				}
				new_s[lvl - 1] = i + 'a';
				new_f[lvl - 1] = i + 'a';
				if (node->children[i]->nrwords != 0) {
					if (lvl < *lung) {
						new_s[lvl] = 0;
						strcpy(sho, new_s);
						*lung = (int)strlen(new_s);
					}
					if (node->children[i]->nrwords > *apar) {
						new_f[lvl] = 0;
						strcpy(frecv, new_f);
						*apar = node->children[i]->nrwords;
					}
				}
				dfs_compl(node->children[i], word, ok, gasit, apar, lvl + 1,
						  lung, small, sho, frecv, new_s, new_f);
			}
		}
	}
}

//Functie de AUTOCOMPLETE
void auto_complete(trie_t *trie, char *word, int k)
{
	char small[256], sho[256], frecv[256], new_s[256], new_f[256];
	int ok = 0, gasit = 0, apar = 0, lung = 256;
	dfs_compl(trie->root, word, &ok, &gasit, &apar, 1, &lung, small,
			  sho, frecv, new_s, new_f);
	if (ok == 0) {
		printf("No words found\n");
		if (k == 0) {
			printf("No words found\n");
			printf("No words found\n");
		}
		return;
	}
	switch (k) {
	case 0:
		printf("%s\n%s\n%s\n", small, sho, frecv);
		break;
	case 1:
		printf("%s\n", small);
		break;
	case 2:
		printf("%s\n", sho);
		break;
	case 3:
		printf("%s\n", frecv);
		break;
	}
}

//Fucntia main care o sa se ocupe de comenzile primite si
//o sa apeleze functile necesare acestora
int main(void)
{
	trie_t *trie = trie_create(ALPHABET_SIZE, ALPHABET);
	char command[256], file_name[256], word[256];
	FILE *f;
	int k;
	while (1) {
		scanf("%s", command);
		if (!strcmp(command, "INSERT")) {
			scanf("%s", word);
			trie_insert(trie, word);
		}
		if (!strcmp(command, "LOAD")) {
			scanf("%s", file_name);
			f = fopen(file_name, "r");
			while (fscanf(f, "%s", word) == 1)
				trie_insert(trie, word);
			fclose(f);
		}
		if (!strcmp(command, "AUTOCORRECT")) {
			scanf("%s", word);
			scanf("%d", &k);
			auto_correct(trie, word, k);
		}
		if (!strcmp(command, "AUTOCOMPLETE")) {
			scanf("%s", word);
			scanf("%d", &k);
			auto_complete(trie, word, k);
		}
		if (!strcmp(command, "REMOVE")) {
			scanf("%s", word);
			trie_remove(trie, word);
		}
		if (!strcmp(command, "EXIT")) {
			trie_free(&trie);
			break;
		}
	}
}
