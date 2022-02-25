# Tema 1 APD 2021 - Paralelizarea unui algoritm genetic (Problema rucsacului)

Torcea Octavian 334CA

* Ma folosesc de o structura "arguments" pentru a putea trimite mai multe date
ca argumente functiei executate de fiecare thread. Aceasta structura contine:
    * vectorul de obiecte;
    * nr de obiecte din sac si capacitatea sacului;
    * nr de generatii pentru care se va executa algoritmul;
    * nr de thread-uri;
    * id-ul fiecarui thread;
    * 2 vectori de indivizi ce reprezinta generatia curenta si pe cea urmatoare;
    * bariera pentru sincronizarea thread-urilor.

* Am modificat functia run_genetic_algorithm astfel incat sa poata sa fie
rulata in paralel pe mai multe thread-uri. Fiecare thread va avea
"responsabilitatea" de a executa functiile specifice algoritmului genetic pe un
nr de indivizi din generatia curenta.

## Rularea funciei `run_genetic_algorithm`
* initializarea generatiei curente si urmatoare in paralel;
apoi, pentru fiecare generatie:
* aplicarea functiei de fitness pe mai multi indivizi in paralel;
* merge-sort paralel pe vectorul de indivizi
* selectarea in paralel a celor mai buni 30% din indivizi;
* aplicarea in paralel a primului tip de mutatie pe primii 20% din indivizi;
* aplicarea in paralel a celui de-al doilea tip de mutatie pe urmatorii 20%
de indivizi;
* aplicarea in paralel a crossover-ului pe primii 30% din indivizi;
* schimbarea generatiei curente cu urmatoarea, realizata pe un singur thread;
* resetarea in paralel a indexului;
* afisarea fitness-ului celui mai bun individ (din 5 in 5 generatii);

## Implementare merge-sort paralel
* vectorul de indivizi este impartit in NR_OF_THREADS parti si fiecare
thread va aplica algoritmul clasic de merge-sort pe bucata sa de vector;
* la final, un singur thread va aplica merge pentru a unifica vectorul.
