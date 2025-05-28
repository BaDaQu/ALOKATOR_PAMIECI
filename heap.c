#include "heap.h"

#define WIELKOSC_CHUNK sizeof(struct memory_chunk_t)
struct memory_manager_t memory_manager;

int heap_setup(void) {
    // Pobranie początkowego adresu sterty i inicjalizacja menedżera pamięci
    memory_manager.poczatek = custom_sbrk(0);
    memory_manager.pierwszy_kawalek = NULL;
    memory_manager.wielkosc_pamieci = 0;

    // Sprawdzenie, czy inicjalizacja się powiodła
    if (memory_manager.poczatek == (void *) -1) {
        return -1;
    }

    return 0;
}

void heap_clean(void) {
    // Sprawdzenie, czy istnieje zaalokowana pamięć do zwolnienia
    if (memory_manager.wielkosc_pamieci > 0) {
        custom_sbrk(-(intptr_t) memory_manager.wielkosc_pamieci);
    }

    // Resetowanie menedżera pamięci do stanu początkowego
    memory_manager.poczatek = NULL;
    memory_manager.pierwszy_kawalek = NULL;
    memory_manager.wielkosc_pamieci = 0;
}

void *heap_malloc(size_t rozmiar) {
    // Sprawdzenie, czy rozmiar jest równy 0 lub czy start pamięci nie jest zainicjalizowany
    if (rozmiar == 0 || memory_manager.poczatek == NULL) {
        return NULL;
    }

    // Pobranie wskaźnika na pierwszy blok pamięci
    struct memory_chunk_t *aktualny_blok = memory_manager.pierwszy_kawalek;

    // Jeśli nie ma jeszcze żadnego bloku, alokuj pierwszy blok pamięci
    if (aktualny_blok == NULL) {
        return malloc_alokuj_poczatkowy_blok_pamieci(rozmiar);
    }

    // Walidacja pamięci przed przeszukiwaniem
    if (heap_validate() != 0) {
        return NULL;
    }

    // Szukaj najlepszego dopasowania lub rozszerz pamięć, jeśli to konieczne
    return malloc_znajdz_najlepsze_dopasowanie_lub_rozszerz_pamiec(rozmiar, aktualny_blok);
}
void *malloc_alokuj_poczatkowy_blok_pamieci(size_t rozmiar) {
    // Rezerwacja miejsca na nowy blok pamięci z dodatkowym miejscem na metadane
    struct memory_chunk_t *blok = (struct memory_chunk_t *) custom_sbrk((long) (WIELKOSC_CHUNK + rozmiar + 8));
    if (blok == (void *) -1) {
        return NULL; // Nie udało się zarezerwować pamięci
    }

    // Inicjalizacja nowego bloku pamięci
    *blok = (struct memory_chunk_t) {.poprzedni = NULL, .nastepny = NULL, .wielkosc = rozmiar, .czy_wolny = 0};

    // Aktualizacja globalnych informacji o zarządzaniu pamięcią
    memory_manager.wielkosc_pamieci += WIELKOSC_CHUNK + rozmiar + 8;
    memory_manager.pierwszy_kawalek = blok;

    // Inicjalizacja bloku pamięci i obliczenie jego checksumy
    malloc_inicjalizuj_blok_pamieci(blok);
    blok->checksuma = oblicz_checksuma(blok);

    // Zwrócenie wskaźnika do użytkowej części bloku pamięci
    return (void *) ((char *) blok + WIELKOSC_CHUNK + 4);
}
void *malloc_znajdz_najlepsze_dopasowanie_lub_rozszerz_pamiec(size_t size, struct memory_chunk_t *blok) {
    struct memory_chunk_t *najlepsze_dopasowanie = NULL;
    size_t najlepszy_rozmiar = SIZE_MAX;

    // Przeszukiwanie listy bloków w poszukiwaniu najlepszego dopasowania
    for (; blok != NULL; blok = blok->nastepny) {
        if (blok->czy_wolny && blok->wielkosc >= size + 8 && blok->wielkosc < najlepszy_rozmiar) {
            najlepsze_dopasowanie = blok;
            najlepszy_rozmiar = blok->wielkosc;
        }

        // Jeśli doszliśmy do końca listy i nie znaleziono dopasowania, rozszerz pamięć
        if (blok->nastepny == NULL && najlepsze_dopasowanie == NULL) {
            return malloc_rozszerz_pamiec(size, blok);
        }
    }

    // Jeśli znaleziono najlepsze dopasowanie, zaktualizuj blok i zwróć wskaźnik do użytkowej części
    if (najlepsze_dopasowanie) {
        najlepsze_dopasowanie->wielkosc = size;
        najlepsze_dopasowanie->czy_wolny = 0;
        malloc_inicjalizuj_blok_pamieci(najlepsze_dopasowanie);
        najlepsze_dopasowanie->checksuma = oblicz_checksuma(najlepsze_dopasowanie);
        return (void *) ((char *) najlepsze_dopasowanie + WIELKOSC_CHUNK + 4);
    }

    return NULL;
}
void malloc_inicjalizuj_nowy_blok(struct memory_chunk_t *blok, struct memory_chunk_t *nowy_blok, size_t rozmiar) {
    // Ustawienie wskaźników i wartości dla nowego bloku
    *nowy_blok = (struct memory_chunk_t) {
            .poprzedni = blok,
            .nastepny = NULL,
            .wielkosc = rozmiar,
            .czy_wolny = 0
    };

    // Aktualizacja wskaźnika next poprzedniego bloku
    blok->nastepny = nowy_blok;

    // Inicjalizacja bloku pamięci i obliczenie metadanych
    malloc_inicjalizuj_blok_pamieci(nowy_blok);
    blok->checksuma = oblicz_checksuma(blok);
    nowy_blok->checksuma = oblicz_checksuma(nowy_blok);
}
void *malloc_rozszerz_pamiec(size_t size, struct memory_chunk_t *blok) {
    // Oblicz potrzebny rozmiar pamięci, uwzględniając dodatkowe bajty na zarządzanie
    size_t calkowity_rozmiar = WIELKOSC_CHUNK + 8 + size;

    // Próba alokacji nowego bloku pamięci
    struct memory_chunk_t *nowy_blok = (struct memory_chunk_t *) custom_sbrk((intptr_t) calkowity_rozmiar);

    // Sprawdzenie, czy alokacja się powiodła
    if (nowy_blok == (void *) -1) {
        return NULL; // Nie udało się zaalokować pamięci
    }

    // Aktualizacja całkowitego rozmiaru zarządzanej pamięci
    memory_manager.wielkosc_pamieci += calkowity_rozmiar;

    // Inicjalizacja nowego bloku pamięci
    malloc_inicjalizuj_nowy_blok(blok, nowy_blok, size);

    // Zwrócenie wskaźnika do użytkowej części pamięci, przesuniętego o odpowiedni offset
    return (void *) ((char *) nowy_blok + WIELKOSC_CHUNK + 4);
}
void malloc_inicjalizuj_blok_pamieci(struct memory_chunk_t *blok) {
    // Ustawienie wskaźnika na koniec bloku danych
    char *koniec_danych = (char *) blok + WIELKOSC_CHUNK + 4 + blok->wielkosc;
    // Ręczne wypełnienie końcowego obszaru znakami '#'
    for (int i = 0; i < 4; i++) {
        koniec_danych[i] = '#';
    }

    // Ustawienie wskaźnika na początek danych
    char *poczatek_danych = (char *) blok + WIELKOSC_CHUNK;
    // Ręczne wypełnienie początkowego obszaru znakami '#'
    for (int i = 0; i < 4; i++) {
        poczatek_danych[i] = '#';
    }
}

void *heap_calloc(size_t liczba, size_t ile) {
    if (liczba == 0 || ile == 0) {
        return NULL;
    }

    size_t wielkosc = liczba * ile;
    void *alokuj = heap_malloc(wielkosc);
    if (alokuj == NULL) {
        return NULL;
    }

    memset(alokuj, 0, wielkosc);
    return alokuj;
}

int sprawdzaj_plotka(struct memory_chunk_t *kawalek) {
    char *poczatek = (char *) kawalek + WIELKOSC_CHUNK;
    char *koniec = (char *) kawalek + 4 + WIELKOSC_CHUNK + kawalek->wielkosc;
    const char expected_fence[4] = {'#', '#', '#', '#'};

    if (memcmp(poczatek, expected_fence, 4) != 0 || memcmp(koniec, expected_fence, 4) != 0) {
        return 0;
    }
    return 1;
}
int heap_validate(void) {
    if (memory_manager.poczatek == NULL) {
        return 2;
    }
    for (struct memory_chunk_t *current_chunk = memory_manager.pierwszy_kawalek;
         current_chunk != NULL;
         current_chunk = current_chunk->nastepny) {
        if (current_chunk->checksuma != oblicz_checksuma(current_chunk)) {
            return 3;
        }

        if (current_chunk->czy_wolny == 0 && !sprawdzaj_plotka(current_chunk)) {
            return 1;
        }
    }

    return 0;
}




void *heap_realloc(void *memblock, size_t count) {
    int wynik = realloc_sprawdz_warunki_poczatkowe(memblock, count);
    if (wynik == 0) {
        return NULL;
    } else if (wynik == 2) {
        return heap_malloc(count);
    }

    struct memory_chunk_t *memory_block = (struct memory_chunk_t *)((char *)memblock - WIELKOSC_CHUNK - 4);
    struct memory_chunk_t *memory_block_next = memory_block->nastepny;

    if (memory_block->wielkosc == count) {
        return memblock;
    }

    return realloc_zdecyduj(memblock, count, memory_block, memory_block_next);
}

int realloc_sprawdz_warunki_poczatkowe(void *blok_pamieci, size_t rozmiar) {
    if (memory_manager.poczatek == NULL || heap_validate() != 0) {
        return 0; // Nieudane sprawdzenie
    }

    if (blok_pamieci == NULL) {
        return 2; // Specjalny przypadek: alokacja nowego bloku
    }

    if (get_pointer_type(blok_pamieci) != pointer_valid) {
        return 0; // Nieudane sprawdzenie
    }

    if (rozmiar == 0) {
        heap_free(blok_pamieci);
        return 0; // Nieudane sprawdzenie
    }

    return 1; // Pomyślne sprawdzenie
}
void *realloc_zdecyduj(void *blok_pamieci, size_t nowy_rozmiar, struct memory_chunk_t *aktualny_blok, struct memory_chunk_t *nastepny_blok) {
    if (nowy_rozmiar < aktualny_blok->wielkosc) {
        return realloc_zmniejsz(blok_pamieci, nowy_rozmiar, aktualny_blok);
    }
    if (nowy_rozmiar > aktualny_blok->wielkosc) {
        if (nastepny_blok != NULL) {
            return realloc_rozszerz_w_miejscu(blok_pamieci, nowy_rozmiar, aktualny_blok, nastepny_blok);
        }
        return realloc_rozszerz_z_sbrk(aktualny_blok, nowy_rozmiar);
    }
    return NULL;
}
void realloc_dostosuj_bloki_pamieci(struct memory_chunk_t *blok_pamieci, struct memory_chunk_t *nastepny_blok_pamieci, size_t nowy_rozmiar) {
    // Ustawienie wskaźników między blokami
    nastepny_blok_pamieci->poprzedni = blok_pamieci;
    blok_pamieci->nastepny = nastepny_blok_pamieci->nastepny;

    // Aktualizacja wielkości bloku
    blok_pamieci->wielkosc = nowy_rozmiar;

    // Ustawienie danych kontrolnych i obliczenie sumy kontrolnej
    char *dane = (char *)blok_pamieci + WIELKOSC_CHUNK + 4 + nowy_rozmiar;
    for (int i = 0; i < 4; i++) {
        dane[i] = '#';
    }
    blok_pamieci->checksuma = oblicz_checksuma(blok_pamieci);
}
void *realloc_przydziel_nowy_blok(struct memory_chunk_t *kawalek_pamieci, size_t rozmiar) {
    // Alokacja nowego bloku pamięci
    void *nowy_blok = heap_malloc(rozmiar);
    if (!nowy_blok) {
        return NULL;
    }

    // Kopiowanie danych do nowego bloku i zwolnienie starego
    memcpy(nowy_blok, (char *)kawalek_pamieci + WIELKOSC_CHUNK + 4, kawalek_pamieci->wielkosc);
    heap_free((char *)kawalek_pamieci + WIELKOSC_CHUNK + 4);

    return nowy_blok;
}
void *realloc_rozszerz_w_miejscu(void *blok_pamieci, size_t rozmiar, struct memory_chunk_t *aktualny_blok, struct memory_chunk_t *nastepny_blok) {
    // Bezpośrednie obliczenie adresu początkowego następnego bloku
    char *poczatek = (char *)nastepny_blok + sizeof(struct memory_chunk_t) + nastepny_blok->wielkosc + 8;

    // Obliczenie całkowitej przestrzeni między blokami
    size_t laczna_przestrzen = (size_t)(poczatek - (char *)aktualny_blok);
    size_t wolna_przestrzen = laczna_przestrzen - WIELKOSC_CHUNK - 8;

    // Warunek sprawdzający czy dostępna przestrzeń jest wystarczająca
    if (wolna_przestrzen >= rozmiar) {
        // Dostosowanie bloków pamięci
        realloc_dostosuj_bloki_pamieci(aktualny_blok, nastepny_blok, rozmiar);
        return blok_pamieci;
    } else {
        // Alokacja nowego bloku pamięci
        return realloc_przydziel_nowy_blok(aktualny_blok, rozmiar);
    }
}
void *realloc_zmniejsz(void *blok_pamieci, size_t nowy_rozmiar, struct memory_chunk_t *kawalek_pamieci) {
    // Ustawienie nowego rozmiaru bloku pamięci
    kawalek_pamieci->wielkosc = nowy_rozmiar;

    // Obliczenie wskaźnika na obszar zaalokowany po strukturze memory_chunk_t
    char *dane = (char *)kawalek_pamieci + sizeof(struct memory_chunk_t) + 4 + kawalek_pamieci->wielkosc;

    // Wypełnienie 4 bajtów po zakończeniu bloku pamięci znakami '#'
    for (int i = 0; i < 4; i++) {
        dane[i] = '#';
    }

    // Obliczenie i zapisanie nowej sumy kontrolnej
    kawalek_pamieci->checksuma = oblicz_checksuma(kawalek_pamieci);

    // Zwrócenie wskaźnika na blok pamięci
    return blok_pamieci;
}
void *realloc_rozszerz_z_sbrk(struct memory_chunk_t blok_pamieci[], size_t nowy_rozmiar) {
    size_t rozmiar = nowy_rozmiar - blok_pamieci->wielkosc;

    // Uzyskaj dodatkową pamięć
    void *pamiec = custom_sbrk((intptr_t)rozmiar);
    if (pamiec == (void *)-1) {
        return NULL;
    }

    // Aktualizuj rozmiar bloku
    blok_pamieci->wielkosc = nowy_rozmiar;
    memory_manager.wielkosc_pamieci += rozmiar;

    // Ustaw nowe ogrodzenie pamięci
    char *dane = (char *)blok_pamieci + 4 + WIELKOSC_CHUNK  + blok_pamieci->wielkosc;
    for (size_t i = 0; i < 4; i++) {
        dane[i] = '#';
    }

    // Zaktualizuj sumę kontrolną
    blok_pamieci->checksuma = oblicz_checksuma(blok_pamieci);

    // Zwróć wskaźnik do nowo przydzielonego bloku pamięci
    char *nowy_adres = (char *)blok_pamieci + 4 + WIELKOSC_CHUNK;
    return (void *)nowy_adres;
}


void heap_free(void *blok_pamieci) {
    // Sprawdzenie, czy blok pamięci jest niepusty i czy menedżer pamięci jest poprawnie zainicjalizowany
    if (!blok_pamieci || heap_validate() != 0 || !memory_manager.poczatek || !memory_manager.pierwszy_kawalek) {
        return;
    }

    // Obliczenie adresu aktualnego bloku pamięci
    struct memory_chunk_t *aktualny_blok = (struct memory_chunk_t *)((char *)blok_pamieci - 4 - WIELKOSC_CHUNK);

    // Sprawdzenie, czy blok jest już zwolniony lub czy jego rozmiar jest większy niż cała dostępna pamięć
    if (aktualny_blok->czy_wolny || aktualny_blok->wielkosc > memory_manager.wielkosc_pamieci) {
        return;
    }

    // Oznaczenie bloku jako wolnego
    aktualny_blok->czy_wolny = 1;

    // Obsługa przypadku, gdy jest to jedyny blok w zarządzanej pamięci
    if (!aktualny_blok->nastepny && !aktualny_blok->poprzedni) {
        memory_manager.pierwszy_kawalek = NULL;
        return;
    }

    // Aktualizacja rozmiaru bloku, jeśli istnieje następny blok
    if (aktualny_blok->nastepny) {
        aktualny_blok->wielkosc = (char *)aktualny_blok->nastepny - (char *)aktualny_blok - WIELKOSC_CHUNK;
    }

    // Scalanie z poprzednim blokiem, jeśli jest wolny
    if (aktualny_blok->poprzedni && aktualny_blok->poprzedni->czy_wolny) {
        free_scal_z_poprzednim(&aktualny_blok);
    }

    // Scalanie z następnym blokiem, jeśli jest wolny
    if (aktualny_blok->nastepny && aktualny_blok->nastepny->czy_wolny) {
        free_scal_z_nastepnym(aktualny_blok);
    }

    // Aktualizacja metadanych dla wszystkich bloków
    for (aktualny_blok = memory_manager.pierwszy_kawalek; aktualny_blok; aktualny_blok = aktualny_blok->nastepny) {
        aktualny_blok->checksuma = oblicz_checksuma(aktualny_blok);
    }
}
void free_scal_z_poprzednim(struct memory_chunk_t **aktualny_blok) {
    if (!aktualny_blok || !*aktualny_blok || !(*aktualny_blok)->poprzedni) {
        return;
    }

    struct memory_chunk_t *poprzedni = (*aktualny_blok)->poprzedni;
    poprzedni->wielkosc += (*aktualny_blok)->wielkosc + WIELKOSC_CHUNK;
    poprzedni->nastepny = (*aktualny_blok)->nastepny;

    if (poprzedni->nastepny) {
        poprzedni->nastepny->poprzedni = poprzedni;
    }

    *aktualny_blok = poprzedni;
}
void free_scal_z_nastepnym(struct memory_chunk_t *aktualny_blok) {
    if (aktualny_blok == NULL || aktualny_blok->nastepny == NULL) {
        return;
    }

    struct memory_chunk_t *nastepny_blok = aktualny_blok->nastepny;
    aktualny_blok->wielkosc += nastepny_blok->wielkosc + WIELKOSC_CHUNK;
    aktualny_blok->nastepny = nastepny_blok->nastepny;

    if (nastepny_blok->nastepny != NULL) {
        nastepny_blok->nastepny->poprzedni = aktualny_blok;
    }
}

size_t heap_get_largest_used_block_size(void) {
    if (!memory_manager.poczatek || !memory_manager.pierwszy_kawalek || heap_validate() != 0) {
        return 0;
    }

    size_t ile = 0;
    for (struct memory_chunk_t *i = memory_manager.pierwszy_kawalek; i; i = i->nastepny) {
        if (i->czy_wolny == 0) {
            if (ile < i->wielkosc ) {
                ile = i->wielkosc;
            }
        }
    }
    return ile;
}
enum pointer_type_t get_pointer_type(const void *const wskaznik) {
    if (wskaznik == NULL) {
        return pointer_null;
    }
    if (memory_manager.pierwszy_kawalek == NULL) {
        return pointer_unallocated;
    }
    if (heap_validate() != 0) {
        return pointer_heap_corrupted;
    }

    for (struct memory_chunk_t *biezacy = memory_manager.pierwszy_kawalek; biezacy != NULL; biezacy = biezacy->nastepny) {
        char *podstawa = (char *) biezacy;
        char *koniec_czesci = podstawa + WIELKOSC_CHUNK;
        char *koniec_plotka = koniec_czesci + 4;
        char *koniec_danych = koniec_plotka + biezacy->wielkosc;
        char *koniec_bloku = koniec_danych + 4;

        if (biezacy->czy_wolny == 0) {
            if (wskaznik < (void *) koniec_czesci) {
                return pointer_control_block;
            }
            if (wskaznik < (void *) koniec_plotka) {
                return pointer_inside_fences;
            }
            if (wskaznik == (void *) koniec_plotka) {
                return pointer_valid;
            }
            if (wskaznik < (void *) koniec_danych) {
                return pointer_inside_data_block;
            }
            if (wskaznik < (void *) koniec_bloku) {
                return pointer_inside_fences;
            }
            if (biezacy->nastepny != NULL) {
                char *nastepny_blok = (char *) biezacy->nastepny;
                if (nastepny_blok > (char *) wskaznik && wskaznik >= (void *) koniec_bloku) {
                    return pointer_unallocated;
                }
            }
        } else {
            if (wskaznik < (void *) koniec_danych) {
                return pointer_unallocated;
            }
        }
    }
    return pointer_unallocated;
}
int oblicz_checksuma(struct memory_chunk_t *memory_block) {
    // Weryfikacja wskaźników
    if (!memory_manager.poczatek || !memory_manager.pierwszy_kawalek || !memory_block) {
        return 0;
    }

    int suma_kontrolna = 0;
    // Użycie wskaźnika do iteracji przez bajty metadanych
    for (unsigned char *i = (unsigned char *) memory_block;
         i < (unsigned char *) memory_block + WIELKOSC_CHUNK - 4; i++) {
        suma_kontrolna += *i;
    }

    return suma_kontrolna;
}
