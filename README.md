# Alokator Pamięci

## Opis Projektu

Projekt ten polega na implementacji własnego menedżera pamięci, który zarządza dynamicznie alokowaną pamięcią na stercie programu. Rozwiązanie dostarcza niestandardowe, autorskie wersje fundamentalnych funkcji alokacji i dealokacji pamięci, znanych ze standardowej biblioteki C, takich jak `heap_malloc`, `heap_calloc`, `heap_realloc` oraz `heap_free`. Celem jest stworzenie mechanizmu zarządzania pamięcią, który nie tylko replikuje podstawowe funkcjonalności, ale także wprowadza dodatkowe mechanizmy bezpieczeństwa i diagnostyki.

### Mechanizm Płotków (Fences)

Jednym z kluczowych założeń i cech charakterystycznych tego alokatora jest implementacja "płotków" (ang. fences). Są to specjalne obszary pamięci, wypełnione znanymi wartościami, umieszczane bezpośrednio przed i bezpośrednio po każdym bloku pamięci przydzielonym użytkownikowi. Głównym zadaniem płotków jest pomoc w wykrywaniu błędów programistycznych polegających na nadpisywaniu pamięci poza granicami alokowanego bloku (tzw. buffer overflow/underflow). Naruszenie (zmiana wartości) płotka sygnalizuje nieprawidłowe użycie pamięci przez kod użytkownika.

### Zarządzanie Stertą i Pozyskiwanie Pamięci

Pamięć wykorzystywana przez stertę jest pozyskiwana od systemu operacyjnego przy użyciu dostarczonej funkcji `custom_sbrk()`. Funkcja ta symuluje standardowe wywołanie systemowe `sbrk()`, pozwalając na dynamiczne rozszerzanie (a także kurczenie) obszaru pamięci dostępnego dla alokatora. Dzięki temu sterta może rosnąć w miarę potrzeb aplikacji, bez konieczności alokowania z góry dużego, stałego fragmentu pamięci.

### Inicjalizacja i Czyszczenie Sterty

Do zarządzania cyklem życia sterty służą dwie podstawowe funkcje: `heap_setup()` oraz `heap_clean()`. Funkcja `heap_setup()` jest odpowiedzialna za inicjalizację wewnętrznych struktur danych alokatora oraz przygotowanie sterty do działania. Z kolei `heap_clean()` ma za zadanie zwolnienie całej pamięci zaalokowanej przez stertę z powrotem do systemu operacyjnego oraz przywrócenie alokatora do stanu początkowego, jakby nie był jeszcze inicjowany. Umożliwia to efektywny reset stanu sterty, co jest szczególnie przydatne w scenariuszach testowych.

### Algorytmy Alokacji i Realokacji

Algorytm alokacji wykorzystywany przez funkcję `heap_malloc` opiera się na strategii "first-fit". Oznacza to, że podczas poszukiwania wolnego bloku pamięci do przydzielenia, wybierany jest pierwszy napotkany blok, którego rozmiar jest wystarczający, aby zaspokoić żądanie użytkownika.

Funkcja `heap_realloc` została zaimplementowana w sposób inteligentny, aby efektywnie zarządzać zmianą rozmiaru wcześniej alokowanych bloków. W miarę możliwości próbuje ona rozszerzyć istniejący blok w miejscu (jeśli za nim znajduje się odpowiednia ilość wolnej przestrzeni). Jeśli rozszerzenie w miejscu nie jest możliwe, alokowany jest nowy, większy blok, zawartość starego bloku jest kopiowana do nowego, a stary blok jest zwalniany.

### Funkcje Diagnostyczne i Pomocnicze

Oprócz podstawowych funkcji alokujących i zwalniających pamięć, projekt zawiera również zestaw funkcji narzędziowych i diagnostycznych. Funkcja `heap_validate()` odgrywa kluczową rolę, pozwalając na weryfikację spójności wewnętrznych struktur danych sterty oraz integralności wspomnianych wcześniej płotków. Jest to istotne narzędzie do debugowania i zapewniania poprawności działania alokatora.

Funkcja `get_pointer_type()` umożliwia klasyfikację przekazanego wskaźnika, określając, czy wskazuje on na poprawny obszar danych użytkownika, strukturę kontrolną, płotek, czy też obszar niezaalokowany lub uszkodzony. Z kolei `heap_get_largest_used_block_size()` dostarcza informacji o rozmiarze największego bloku aktualnie przydzielonego użytkownikowi.

Cała implementacja dąży do jak najwierniejszego odwzorowania zachowania standardowych funkcji z rodziny `malloc`, kładąc jednocześnie duży nacisk na mechanizmy wykrywania potencjalnych uszkodzeń sterty i niepoprawnego użycia pamięci.
