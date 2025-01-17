# Algorytm
Zaimplementowany algorytm polega na wyzanczaniu skrótów md5 wiadomości, będących kolejnymi kombinacjami o pewnej długości z ustalonego alfabetu, oraz porównywania ich to oczekiwanej wartości. Wykorzystano gotową implementację funkcji `md5`, dostępna pod adresem https://github.com/Zunawe/md5-c/blob/main/md5.c .

Dane wejściowe:
 - długość wiadomości, np 8
 - alfabet, np: `abcdefghijklmnopqrstuvwxyz` - 26 znaków
 - skrót md5 szukanej wiadomości, np `md5("pomidory") = 9e65ff77204283d4a951e12d2bb8357e`
 - ilosc wiadomości do sprawdzenia, np 1000000 (na potrzeby łatwego badania wydajności), osiągnięcie podanej ilości sprawdzonych wiadomości powoduje wczesne zakończenie programu

Dodatkowe stałe:
 - rozmar partii, np 65536

Dane wyjściowe:
 - wiadomosc -  pierwsza znaleziona wiadomość której skrót jest taki sam jak skrót szukanej wiadomości
 - brak wyniku - wiadomość o takim samym skrócie nie została znaleziona

## Cel zrównoleglania
Celem zrównoleglania tego algorytmu jest skrócenie czasu oczekiwania na sprawdzenie wszystkich możliwych kombinacji.

# Opis implementacji

## Wersja sekwencyjna
```c
struct search_result check_hashes(size_t length, size_t hashes_to_check, uint8_t orginal_hash[16]) {

    struct search_result search_result = {.password = NULL,
                                          .checked_passwords = 0};

    int finished = 0;
    size_t total_checked_hashes = 0;

    // podział na partie o rozmiarze BATCH_SIZE
    for (size_t n = 0; n <= hashes_to_check / BATCH_SIZE; n++) {
        if (finished == 1) {
            continue;
        }

        size_t combination_number = BATCH_SIZE * n;
        size_t to_check = min(BATCH_SIZE, hashes_to_check - combination_number);

        search_result = check_batch(combination_number, length, to_check, orginal_hash);
        total_checked_hashes += search_result.checked_passwords;

        if (search_result.password != NULL) {
            finished = 1;
            printf("finished: %p\n", search_result.password);
        }
        if (total_checked_hashes > hashes_to_check) {
            finished = 1;
        }
    }

    return search_result;
}

struct search_result check_batch(size_t combination_number, size_t length,
                                 size_t hashes_to_check, uint8_t orginal_hash[16]) {
    char *password = NULL;
    size_t indices[PASSWORD_LENGTH] = {};
    get_nth_combinaiton(indices, combination_number, length);

    int pos = length - 1;
    char buffer[PASSWORD_LENGTH + 1] = {0};
    uint8_t result[16];

    size_t checked_hashes = 0;
    while (checked_hashes < hashes_to_check && pos >= 0) {
        // ustawienie kolejnej kombinacji z powtórzeniami
        next_combination(buffer, indices, &pos);

        // wyznaczenie skrótu kombinacji
        md5String(buffer, length, result);
        checked_hashes++;

        // sprawdzanie czy skróty się zgadzają
        if (memcmp(orginal_hash, result, 16) == 0) {
            // znaleziono pasuąjącą wiadomość - koniec algorytmu
            password = clone(buffer);
            break;
        }
        // czyszczenie wiadomosci
        memset(buffer, 0, PASSWORD_LENGTH);
    }

    struct search_result search_result = {.password = password,
                                          .checked_passwords = checked_hashes};

    return search_result;
}
```

## Wersja zrównoleglona `OpenMP`

Zrównolegle algorytmu jednowątkowego przy użyciu biblioteki OpenMP polega na rówomiernym rozdzieleniu partii wiadomości do sprawdzenia między dostępne wątki, w tym celu użyto dyrektywy `#pragma omp parallel for`.

```c
struct search_result check_hashes(size_t length, size_t hashes_to_check, uint8_t orginal_hash[16]) {

    struct search_result search_result = {.password = NULL,
                                          .checked_passwords = 0};

    int finished = 0;
    size_t total_checked_hashes = 0;

    // podział na partie o rozmiarze BATCH_SIZE
#pragma omp parallel for shared(total_checked_hashes, finished)
    for (size_t n = 0; n <= hashes_to_check / BATCH_SIZE; n++) {
        if (finished == 1) {
            continue;
        }

        size_t combination_number = BATCH_SIZE * n;
        size_t to_check = min(BATCH_SIZE, hashes_to_check - combination_number);

        search_result = check_batch(combination_number, length, to_check, orginal_hash);
#pragma omp atomic
        total_checked_hashes += search_result.checked_passwords;

        if (search_result.password != NULL) {
            finished = 1;
            printf("finished: %p\n", search_result.password);
        }
    }

    search_result.checked_passwords = total_checked_hashes;

    return search_result;
}

struct search_result check_batch(size_t combination_number, size_t length,
                                 size_t hashes_to_check, uint8_t orginal_hash[16]) {
   // taka sama implementacja jak przypadku OpenMP
}
```

## Wersja zrównolegloa `MPI`

Zaimplementowane zrównoleglenie algorytmu przy pomocy MPI rozdziela problem na równe części, które są następnie sprawdzane niezależnie przez każdy proces. Sposób przydziału wiadomości dla każdego z procesów polega na wyznaczeniu numerów pierwszej i ostatniej kombinacji na podstawie numeru procesu oraz ilości wiadomości do sprawdzenia.

```c
void run_tests(int rank, int size, size_t hashes_to_check, uint8_t orginal_hash[16]) {
    size_t hashes_per_process = hashes_to_check / size;

    // wyznaczanie pierwszej i ostatniej kombinacji dla danego procesu
    size_t start = rank * hashes_per_process;
    size_t end = (rank == size - 1) ? hashes_to_check
                                    : (start + hashes_per_process);
    size_t total_checked_hashes = 0;

    struct search_result search_result = {.password = NULL,
                                          .checked_passwords = 0};

    for (size_t combination_number = start; combination_number < end;
         combination_number += BATCH_SIZE) {
        size_t to_check = min(BATCH_SIZE, end - combination_number);

        search_result =
            check_batch(combination_number, PASSWORD_LENGTH, to_check, orginal_hash);
        total_checked_hashes += search_result.checked_passwords;

        if (search_result.password != NULL) {
            break;
        }
    }

    if (search_result.password != NULL) {
        printf("Process %d found the password: %s\n", rank,
               search_result.password);
    }
}

struct search_result check_batch(size_t combination_number, size_t length,
                                 size_t hashes_to_check, uint8_t orginal_hash[16]) {
   // taka sama implementacja jak przypadku OpenMP
}
```

## Wersja zrównoleglona `MPI + OpenMP`


Work in progress...


# Sposób wykonania pomiarów

Każdy z programów miał za zadanie obliczyć skróty 16777216 wiadomości oraz porównać je do losowego skrótu, takie podejście pozwala uzyskać powtarzalne wyniki oraz zapewnia równy rozmiar problemu dla każdego uruchomienia programów. Został wybrany losowy skrót, aby uniemożliwić wczesne zakończenie działania programu.

Pomiary zostały zebrane z pomocą skryptów wielokrotnie uruchamiających programy odpowiadające każdej z metod zrównoleglenia. Warianty programów OpenMP oraz MPI. Każda z konfiguracji wątków dla każdego wariantu programu została uruchomiona 10 razy, ma to na celu zredukowanie różnic w wydajności między uruchomieniami programów.

## Jednowątkowo

Czas wykonania to całkowity czas trwania sprawdzania pętli odpowiadającej za podział na partie oraz sprawdzanie tych partii.

## OpenMP

Czas wykonania to całkowity czas trwania sekcji zrównoleglonej przy pomocy dyrektywy `#pragma omp parallel for`, to oznacza, że pomiary zawierają narzut interfejsu OpenMP.

## MPI

Czas wykonania to suma pomiarów czasów wykonania jednowątkowych części algorytmów, to oznacza, że nie jest w nie wliczany narzut spowodowany środowiskiem MPI.

## MPI + OpenMP

Czas wykonania to suma pomiarów czasów wykonania sekcji zrównoleglonych wykonywanych w każdym z procesów. Pomiary nie zawierają narzutu spowodowanego środowiskiem MPI, natomiast zawierają narzut OpenMP.

# Parametry środowiska uruchomieniowego

Pomiary zostały zebrane na serwerze torus. Serwer ten zawiera 64-bitowy, 32 rdzeniowy procesor Intel 6 generacji. System operacyjny to Debian GNU/Linux 11 (bullseye). Wersja MPI to 4.1.0, wersja gcc to 8.5, biblioteka OpenMP jest zawarta w kompilatorze gcc. Poniżej wynik poleceń pozwalających na pobranie informacji o maszynie.

```
niedzielski.lukasz@torus:~$ lscpu
Architecture:                    x86_64
CPU op-mode(s):                  32-bit, 64-bit
Byte Order:                      Little Endian
Address sizes:                   40 bits physical, 48 bits virtual
CPU(s):                          32
On-line CPU(s) list:             0-31
Thread(s) per core:              1
Core(s) per socket:              1
Socket(s):                       32
NUMA node(s):                    1
Vendor ID:                       GenuineIntel
CPU family:                      6
Model:                           85
Model name:                      Intel Xeon Processor (Skylake, IBRS)
Stepping:                        4
CPU MHz:                         2294.608
BogoMIPS:                        4589.21
Hypervisor vendor:               KVM
Virtualization type:             full
L1d cache:                       1 MiB
L1i cache:                       1 MiB
L2 cache:                        128 MiB
L3 cache:                        512 MiB
NUMA node0 CPU(s):               0-31
Vulnerability Itlb multihit:     KVM: Mitigation: VMX unsupported
Vulnerability L1tf:              Mitigation; PTE Inversion
Vulnerability Mds:               Mitigation; Clear CPU buffers; SMT Host state unknown
Vulnerability Meltdown:          Mitigation; PTI
Vulnerability Spec store bypass: Mitigation; Speculative Store Bypass disabled via prctl and sec
                                 comp
Vulnerability Spectre v1:        Mitigation; usercopy/swapgs barriers and __user pointer sanitiz
                                 ation
Vulnerability Spectre v2:        Mitigation; Retpolines, IBPB conditional, IBRS_FW, STIBP disabl
                                 ed, RSB filling
Vulnerability Srbds:             Not affected
Vulnerability Tsx async abort:   Not affected
Flags:                           fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov p
                                 at pse36 clflush mmx fxsr sse sse2 ss syscall nx pdpe1gb rdtscp
                                  lm constant_tsc rep_good nopl xtopology cpuid tsc_known_freq p
                                 ni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe pop
                                 cnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lah
                                 f_lm abm 3dnowprefetch cpuid_fault invpcid_single pti ssbd ibrs
                                  ibpb fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid mpx 
                                 avx512f avx512dq rdseed adx smap clflushopt clwb avx512cd avx51
                                 2bw avx512vl xsaveopt xsavec xgetbv1 xsaves arat umip pku ospke
                                  avx512_vnni md_clear

```

```
niedzielski.lukasz@torus:~$ free -h
               total        used        free      shared  buff/cache   available
Mem:            62Gi        49Gi       8.7Gi        17Mi       5.0Gi        12Gi
Swap:           59Gi        14Gi        45Gi
```

```
niedzielski.lukasz@torus:~$ lsb_release -a
No LSB modules are available.
Distributor ID:	Debian
Description:	Debian GNU/Linux 11 (bullseye)
Release:	11
Codename:	bullseye
```


```
niedzielski.lukasz@torus:~$ mpiexec --version
mpiexec (OpenRTE) 4.1.0

Report bugs to http://www.open-mpi.org/community/help/
```

```
niedzielski.lukasz@torus:~$ gcc --version
gcc (GCC) 8.5.0
Copyright (C) 2018 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```




# Wnioski

Najlepsze zrównoleglenie osiągnięto wykorzystując ...

Kombinacja MPI + OpenMP wykazała najgorsze wyniki, jednak należy zauważyć, że nie są one znacznie gorsze od wyników OpenMP. W takiej konfiguracji nakładają się narzuty z obu wykorzystanych narzędzi, co jest jedną z przyczyn gorszej wydajności względem wariantów OpenMP i MPI. Nie oznacza to jednak, że nie należy jej stosować, taka konfiguracja pozwala zwiększyć całkowite dostępne zasoby obliczeniowe poprzez łączenie zasobów wielu maszyn, co może przełożyć się na poprawę wydajności algorytmu. 


