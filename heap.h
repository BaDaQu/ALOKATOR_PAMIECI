#ifndef BADAQU_HEAP_H
#define BADAQU_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

struct memory_manager_t {
    void *poczatek;
    size_t wielkosc_pamieci;
    struct memory_chunk_t *pierwszy_kawalek;
};

struct memory_chunk_t {
    struct memory_chunk_t* poprzedni;
    struct memory_chunk_t* nastepny;
    size_t wielkosc;
    int czy_wolny;
    int checksuma;
};

enum pointer_type_t {
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

struct memory_manager_t memory_manager;

int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t liczba, size_t ile);
void* heap_realloc(void* memblock, size_t count);
void heap_free(void* memblock);
int heap_validate(void);
size_t heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);


//POMOCNICZE
int oblicz_checksuma(struct memory_chunk_t *memory_block);

//MALLOC
void *malloc_alokuj_poczatkowy_blok_pamieci(size_t rozmiar);
void *malloc_znajdz_najlepsze_dopasowanie_lub_rozszerz_pamiec(size_t size, struct memory_chunk_t *blok);
void malloc_inicjalizuj_nowy_blok(struct memory_chunk_t *blok, struct memory_chunk_t *nowy_blok, size_t rozmiar);
void *malloc_rozszerz_pamiec(size_t size, struct memory_chunk_t *blok);
void malloc_inicjalizuj_blok_pamieci(struct memory_chunk_t *blok);

//REALLOC
int realloc_sprawdz_warunki_poczatkowe(void *blok_pamieci, size_t rozmiar);
void *realloc_zdecyduj(void *blok_pamieci, size_t nowy_rozmiar, struct memory_chunk_t *aktualny_blok, struct memory_chunk_t *nastepny_blok);
void realloc_dostosuj_bloki_pamieci(struct memory_chunk_t *blok_pamieci, struct memory_chunk_t *nastepny_blok_pamieci, size_t nowy_rozmiar);
void *realloc_przydziel_nowy_blok(struct memory_chunk_t *kawalek_pamieci, size_t rozmiar);
void *realloc_rozszerz_w_miejscu(void *blok_pamieci, size_t rozmiar, struct memory_chunk_t *aktualny_blok, struct memory_chunk_t *nastepny_blok);
void *realloc_zmniejsz(void *blok_pamieci, size_t nowy_rozmiar, struct memory_chunk_t *kawalek_pamieci);
void *realloc_rozszerz_z_sbrk(struct memory_chunk_t *memory_block, size_t count);

//VALIDATE
int sprawdzaj_plotka(struct memory_chunk_t *kawalek);

//FREE
void free_scal_z_poprzednim(struct memory_chunk_t **aktualny_blok);
void free_scal_z_nastepnym(struct memory_chunk_t *aktualny_blok);


#endif //BADAQU_HEAP_H
