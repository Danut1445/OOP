//Copyright Tunsoiu Dan-Andrei 315CA 2022-2023
#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

//Mai jos avem structurile pentru headul listei
//de blocuiri, blocurile in sine care o sa fie
//fiecare si un head pentru liste de miniblocuri
//si miniblocurile in sine
typedef struct nodmini nodmini;
struct nodmini {
	uint64_t address;
	size_t size;
	uint8_t perm;
	void *data;
	nodmini *next, *prev;
};

typedef struct lstmini lstmini;
struct lstmini {
	lstmini *next;
	lstmini *prev;
	uint64_t address;
	size_t size;
	nodmini *head;
};

typedef struct lstblock lstblock;
struct lstblock {
	uint64_t nrblock;
	uint64_t nrminiblock;
	uint64_t arenasize;
	uint64_t freememory;
	lstmini *head;
};

//Functia de alloc_arena va crea headul listei de
//blocuri si il va returna ca un pointer
lstblock *alloc_arena(const uint64_t dimensions_arena)
{
	lstblock *blocklist = malloc(sizeof(lstblock));
	if (!blocklist) {
		printf("ERROARE LA ALOCARE\n");
		return NULL;
	}
	blocklist->nrblock = 0;
	blocklist->nrminiblock = 0;
	blocklist->arenasize = dimensions_arena;
	blocklist->freememory = dimensions_arena;
	blocklist->head = NULL;
	return blocklist;
}

//Dealoc arena elibereaza toata memoria
//alocata de lista de blocuri si inclusiv
//pe aceasta
void dealloc_arena(lstblock *blocklist)
{
	lstmini *act = blocklist->head;
	nodmini *actmini;
	while (act) {
		actmini = act->head;
		while (actmini) {
			act->head = actmini->next;
			free(actmini->data);
			free(actmini);
			actmini = act->head;
		}
		blocklist->head = act->next;
		free(act);
		act = blocklist->head;
	}
	free(blocklist);
}

//Aceasta functie este utilizata pentru a aloca un block
//cu o anumita adresa si un anumit size
lstmini *miniblocklist(const uint64_t address, const uint64_t size)
{
	lstmini *lstminiblock = malloc(sizeof(lstmini));
	if (!lstminiblock) {
		printf("ERROARE LA ALOCARE\n");
		return NULL;
	}
	lstminiblock->address = address;
	lstminiblock->size = size;
	return lstminiblock;
}

//Asemanator celei anterioare, doar ca aloca un miniblock
nodmini *miniblock(const uint64_t address,
				   const uint64_t size)
{
	nodmini *nod = malloc(sizeof(nodmini));
	if (!nod) {
		printf("ERROARE LA ALOCARE\n");
		return NULL;
	}
	nod->address = address;
	nod->size = size;
	nod->perm = 6;
	nod->next = NULL;
	nod->prev = NULL;
	nod->data = malloc(size);
	for (uint64_t i = 0; i < size; i++)
		((char *)nod->data)[i] = 0;
	return nod;
}

//Interconectare verifica daca blocul dat ca paramentu
//Are adresa finala egala cu adresa de inceput al urmatorului
//bloc si daca da atunci le uneste intr-un singur bloc
void interconectare(lstmini *act, lstblock *blocklist)
{
	if (act->next) {
		if (act->address + act->size == act->next->address) {
			lstmini *aux;
			act->size = act->size + act->next->size;
			nodmini *nodact = act->head;
			while (nodact->next)
				nodact = nodact->next;
			nodact->next = act->next->head;
			act->next->head->prev = nodact;
			aux = act->next;
			act->next = act->next->next;
			if (act->next)
				act->next->prev = act;
			free(aux);
			blocklist->nrblock--;
		}
	}
}

//Search cauta blocul cu cea mai mica dresa mai mare decat
//o anumita adresa data ca parametru
lstmini *search(lstblock *blocklist, const uint64_t address)
{
	lstmini *act = blocklist->head;
	if (act) {
		while (act->next && blocklist->nrblock) {
			if (act->next->address > address)
				break;
			act = act->next;
		}
	}
	return act;
}

void freealloc(lstmini *lstminiblock, nodmini *nod)
{
	free(lstminiblock);
	free(nod->data);
	free(nod);
}

//Aloc minibloc, va verifica daca adresa si sizeul primite sunt
//valide si dupa aceea va verifica daca exista un bloc caruia sa
//i se adauge miniblocul pe care o sa il creem, daca nu exista
//atunci va dauga un block nou la lsita de blocuri si in el va
//initializa noul miniblock, pe langa acestea aceasta va apela
//si functia de interconectare
void alloc_miniblock(lstblock *blocklist, const uint64_t address,
					 const uint64_t size)
{
	lstmini *act = search(blocklist, address);
	if (!act) {
		lstmini *lstminiblock = miniblocklist(address, size);
		lstminiblock->head = miniblock(address, size);
		lstminiblock->prev = NULL;
		lstminiblock->next = NULL;
		blocklist->head = lstminiblock;
		blocklist->nrblock = 1;
		blocklist->nrminiblock = 1;
		blocklist->freememory -= size;
		return;
	}
	if (act->address == address) {
		printf("This zone was already allocated.\n");
		return;
	}
	if (act->address + act->size > address && act->address < address) {
		printf("This zone was already allocated.\n");
		return;
	}
	if (act->next) {
		if (act->next->address < address + size) {
			printf("This zone was already allocated.\n");
			return;
		}
	} else {
		if (blocklist->arenasize <= address) {
			printf("The allocated address is outside the size of arena\n");
			return;
		}
		if (blocklist->arenasize < address + size) {
			printf("The end address is past the size of the arena\n");
			return;
		}
	}
	if (act->address + act->size == address) {
		nodmini *nod = miniblock(address, size);
		act->size = act->size + size;
		nodmini *nodact = act->head;
		while (nodact->next)
			nodact = nodact->next;
		nodact->next = nod;
		nod->prev = nodact;
		blocklist->nrminiblock++;
	} else {
		nodmini *nod = miniblock(address, size);
		lstmini *lstminiblock = miniblocklist(address, size);
		if (address == blocklist->head->address && size) {
			printf("This zone was already allocated.\n");
			freealloc(lstminiblock, nod);
			return;
		}
		if (address < blocklist->head->address) {
			if (blocklist->head->address < address + size) {
				printf("This zone was already allocated.\n");
				freealloc(lstminiblock, nod);
				return;
			}
			act->prev = lstminiblock;
			blocklist->head = lstminiblock;
			lstminiblock->next = act;
			lstminiblock->prev = NULL;
		} else {
			if (act->next)
				act->next->prev = lstminiblock;
			lstminiblock->next = act->next;
			lstminiblock->prev = act;
			act->next = lstminiblock;
		}
		lstminiblock->head = nod;
		blocklist->nrminiblock++;
		blocklist->nrblock++;
		act = lstminiblock;
	}
	interconectare(act, blocklist);
	blocklist->freememory -= size;
}

//Free minibloc va elibera un minibloc in functie de pozitia acestuia
//Cazul mai special si mai complex fiind acela in care miniblocul
//se afla in mijlocul unei liste de miniblocuir
void free_miniblock(lstblock *blocklist, const uint64_t address)
{
	lstmini *act;
	nodmini *actmini;
	act = blocklist->head;
	while (act) {
		if (act->address <= address && act->address + act->size > address)
			break;
		act = act->next;
	}
	if (!act) {
		printf("Invalid address for free.\n");
		return;
	}
	actmini = act->head;
	while (actmini) {
		if (actmini->address == address)
			break;
		actmini = actmini->next;
	}
	if (!actmini) {
		printf("Invalid address for free.\n");
		return;
	}
	if (!actmini->prev) {
		act->head = actmini->next;
		if (actmini->next) {
			actmini->next->prev = NULL;
			act->address = actmini->next->address;
		}
		act->size -= actmini->size;
	} else {
		if (!actmini->next) {
			actmini->prev->next = NULL;
			act->size -= actmini->size;
		} else {
			lstmini *newblock = malloc(sizeof(lstmini));
			newblock->head = actmini->next;
			actmini->next->prev = NULL;
			actmini->prev->next = NULL;
			newblock->address = actmini->next->address;
			newblock->size = act->address + act->size - newblock->address;
			newblock->next = act->next;
			if (act->next)
				act->next->prev = newblock;
			newblock->prev = act;
			act->next = newblock;
			act->size = actmini->address - act->address;
			blocklist->nrblock++;
		}
	}
	if (!act->head) {
		if (!act->prev)
			blocklist->head = act->next;
		else
			act->prev->next = act->next;
		if (act->next)
			act->next->prev = act->prev;
		free(act);
		blocklist->nrblock--;
	}
	blocklist->freememory += actmini->size;
	blocklist->nrminiblock--;
	free(actmini->data);
	free(actmini);
}

//Funcita write va cauta mai inati blocul in care se afla
//adresa data ca parametru, apoi cauta miniblocul si apoi
//verifica permisiunile tuturor adreselor pe care urmeaza
//sa se scrie data, iar daca nu exista suficiente adresa
//va scrie cate sunt posibile si va afisa un mesaj legat
//de acest lucru
void write(lstblock *blocklist, const uint64_t address,
		   const uint64_t size, uint8_t *data)
{
	lstmini *act;
	nodmini *actmini, *copieactmini;
	act = blocklist->head;
	while (act) {
		if (act->address <= address && act->address + act->size > address)
			break;
		act = act->next;
	}
	if (!act) {
		printf("Invalid address for write.\n");
		return;
	}
	actmini = act->head;
	while (actmini) {
		if (actmini->address <= address &&
			actmini->address + actmini->size > address)
			break;
		actmini = actmini->next;
	}
	if (!actmini) {
		printf("Invalid address for write.\n");
		return;
	}
	copieactmini = actmini;
	size_t writtensize = 0, writesize, adroffset;
	while (copieactmini && writtensize < size) {
		if (!(copieactmini->perm % 4 / 2)) {
			printf("Invalid permissions for write.\n");
			return;
		}
		if (copieactmini->address < address)
			adroffset = address - copieactmini->address;
		else
			adroffset = 0;
		if (copieactmini->size - adroffset > size - writtensize)
			writesize = size - writesize;
		else
			writesize = copieactmini->size - adroffset;
		writtensize += writesize;
		copieactmini = copieactmini->next;
	}
	writtensize = 0;
	while (actmini && writtensize < size) {
		if (actmini->address < address)
			adroffset = address - actmini->address;
		else
			adroffset = 0;
		if (actmini->size - adroffset > size - writtensize)
			writesize = size - writtensize;
		else
			writesize = actmini->size - adroffset;
		memcpy(actmini->data + adroffset, data + writtensize, writesize);
		writtensize += writesize;
		actmini = actmini->next;
	}

	if (writtensize < size) {
		printf("Warning: size was bigger than the block size.");
		printf(" Writing %ld characters.\n", writtensize);
	}
}

//Functia read este aproape identica celei de write
void read(lstblock *blocklist, const uint64_t address, const uint64_t size)
{
	lstmini *act;
	nodmini *actmini, *copieactmini;
	act = blocklist->head;
	while (act) {
		if (act->address <= address &&
			act->address + act->size > address)
			break;
		act = act->next;
	}
	if (!act) {
		printf("Invalid address for read.\n");
		return;
	}
	actmini = act->head;
	while (actmini) {
		if (actmini->address <= address &&
			actmini->address + actmini->size > address)
			break;
		actmini = actmini->next;
	}
	if (!actmini) {
		printf("Invalid address for read.\n");
		return;
	}
	size_t readsize = 0, toreadsize, adroffset;
	if (act->address + act->size - address < size) {
		readsize = act->address + act->size - address;
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %ld characters.\n", readsize);
	}
	readsize = 0;
	copieactmini = actmini;
	while (copieactmini && readsize < size) {
		if (!(copieactmini->perm / 4)) {
			printf("Invalid permissions for read.\n");
			return;
		}
		if (copieactmini->address < address)
			adroffset = address - copieactmini->address;
		else
			adroffset = 0;
		if (copieactmini->size - adroffset > size - readsize)
			toreadsize = size - readsize;
		else
			toreadsize = copieactmini->size - adroffset;
		readsize += toreadsize;
		copieactmini = copieactmini->next;
	}
	readsize = 0;
	while (actmini && readsize < size) {
		if (actmini->address < address)
			adroffset = address - actmini->address;
		else
			adroffset = 0;
		if (actmini->size - adroffset > size - readsize)
			toreadsize = size - readsize;
		else
			toreadsize = actmini->size - adroffset;
		readsize += toreadsize;
		for (size_t i = 0; i < toreadsize; i++)
			if (((char *)actmini->data + adroffset)[i])
				printf("%c", ((char *)actmini->data + adroffset)[i]);
			else
				break;
		actmini = actmini->next;
	}
	printf("\n");
}

//Functia mprotect va atribui unui anumit minibloc permisiunile date
void mprotect(lstblock *blocklist, uint64_t address, uint8_t *permissions)
{
	char *perm;
	int prm = 0;
	perm = strtok((char *)permissions, " |\n");
	lstmini *act;
	nodmini *actmini;
	act = blocklist->head;
	while (act) {
		if (act->address <= address && act->address + act->size > address)
			break;
		act = act->next;
	}
	if (!act) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	actmini = act->head;
	while (actmini) {
		if (actmini->address == address)
			break;
		actmini = actmini->next;
	}
	if (!actmini) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	while (perm) {
		if (!strcmp(perm, "PROT_NONE"))
			prm = 0;
		if (!strcmp(perm, "PROT_EXEC"))
			prm = prm | 1;
		if (!strcmp(perm, "PROT_WRITE"))
			prm = prm | 2;
		if (!strcmp(perm, "PROT_READ"))
			prm = prm | 4;
		perm = strtok(NULL, " |\n");
	}
	actmini->perm = prm;
}

//Functia PMAP afiseaza lista de blocuri si pentru fiecare bloc
//imparte lista de miniblocuri cu adresa de inceput si cea de final
void pmap(lstblock *blocklist)
{
	uint64_t nr1 = 1, nr2 = 1;
	lstmini *act = blocklist->head;
	nodmini *actmini;
	printf("Total memory: 0x%lX bytes\n", blocklist->arenasize);
	printf("Free memory: 0x%lX bytes\n", blocklist->freememory);
	printf("Number of allocated blocks: %ld\n", blocklist->nrblock);
	printf("Number of allocated miniblocks: %ld\n", blocklist->nrminiblock);
	while (act) {
		printf("\n");
		printf("Block %ld begin\n", nr1);
		printf("Zone: 0x%lX - 0x%lX\n", act->address, act->address + act->size);
		actmini = act->head;
		nr2 = 1;
		while (actmini) {
			printf("Miniblock %ld:\t\t0x%lX\t\t-\t\t0x%lX\t\t| ",
				   nr2, actmini->address, actmini->address + actmini->size);
			if (actmini->perm / 4)
				printf("R");
			else
				printf("-");
			if (actmini->perm % 4 / 2)
				printf("W");
			else
				printf("-");
			if (actmini->perm % 2)
				printf("X");
			else
				printf("-");
			printf("\n");
			actmini = actmini->next;
			nr2++;
		}
		printf("Block %ld end\n", nr1);
		act = act->next;
		nr1++;
	}
}

//In main se face citirea comenzii si apoi a parametrilor
//in functie de aceasta
int main(void)
{
	lstblock *blocklist;
	char command[60];
	uint8_t *data;
	int ok = 0;
	uint64_t address;
	size_t size;
	while (1) {
		ok =  0;
		scanf("%s", command);
		if (!strcmp(command, "ALLOC_BLOCK")) {
			ok = 1;
			scanf("%lu %lu", &address, &size);
			alloc_miniblock(blocklist, address, size);
		}
		if (!strcmp(command, "ALLOC_ARENA")) {
			ok = 1;
			scanf("%lu", &size);
			blocklist = alloc_arena(size);
		}
		if (!strcmp(command, "PMAP")) {
			ok = 1;
			pmap(blocklist);
		}
		if (!strcmp(command, "DEALLOC_ARENA")) {
			ok = 1;
			dealloc_arena(blocklist);
			break;
		}
		if (!strcmp(command, "FREE_BLOCK")) {
			ok = 1;
			scanf("%lu", &address);
			free_miniblock(blocklist, address);
		}
		if (!strcmp(command, "WRITE")) {
			ok = 1;
			scanf("%lu %lu", &address, &size);
			data = malloc(size + 1);
			getchar();
			for (size_t i = 0; i < size; i++)
				data[i] = getchar();
			data[size] = 0;
			//printf("%s\n", data);
			write(blocklist, address, size, data);
			free(data);
		}
		if (!strcmp(command, "READ")) {
			ok = 1;
			scanf("%ld %ld", &address, &size);
			read(blocklist, address, size);
		}
		if (!strcmp(command, "MPROTECT")) {
			ok = 1;
			scanf("%lu", &address);
			data = malloc(51);
			size = 50;
			getchar();
			getline((char **)&data, &size, stdin);
			mprotect(blocklist, address, data);
			free(data);
		}
		if (!ok)
			printf("Invalid command. Please try again.\n");
	}
}
