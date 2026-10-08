section .bss

;Z uwagi na to, ze moj algorytm dziala gdy dlugosc Q rowna jest dlugosci X
;To potrzebuje moc przerzucic Q tymczasowo do wiekszej pamieci
wieksze_Q:
    resq 8000

section .text

;Makro to przechodzi po wszystkich argumentach
;po kolei i wrzuca je na stos
%macro zachowaj 1-*
    %rep %0

    push %1
    %rotate 1

    %endrep
%endmacro

;Makro to przechodzi po wszystkich argumentach
;w odwrotnej kolejnosci i zdejmuje je ze stosu
%macro przywroc 1-*
    %rep %0

    %rotate -1
    pop %1

    %endrep
%endmacro

;Stale, z ktorych korzysta program
%define LOG_64 6
;W bajtach
%define DLUGOSC_SLOWA 8
global nsqrt

nsqrt:
    ;rcx = licznik
    ;r11 = 2n-2j
    ;rdi = Q pointer
    ;rsi = X pointer
    ;rdx = liczba bitow w X (na poczatku w Q)
    ;rbx = wartość logiczna odpowiadająca wyrażeniu R<T
    ;rax = j, na poczatku = floor(n/2) + 1 (+1 z inc rax)
    ;r10 = wskaznik na nowe Q
    ;r8 = wskaznik na X
    ;r9 = rejestr gdzie trzymam adres prawdziwego Q
    ;rbp = liczba slow w X
    zachowaj rbp, rbx

;Niestety pierwotnie nie doczytałem treści i napisałem kod,
;który zakłada, że Q ma zaalokowane tyle samo pamięci co X
;Rozwiązanie to nie korzystało z żadnej dodatkowej pamięci
;Dlatego też biorę "nowe Q" mające 2 razy więcej pamięci
;I odpalam z nim moje pierwotne rozwiązanie
;(stąd wynika dziwna struktura mojego kodu)
;Chce ustawic paramtery tak, zeby wywolac z uzyciem:
;nsqrt(wieksze_Q, X, 2*n)

;Wyjaśnię mój algorytm (należy założyć od tej pory że jako n rozumiem 2n):
;Niech Q'(j-1) = 2^(n-j+1)*Q(j-1)
;Wtedy T(j-1) = Q'(j) + 4^(n-j) = Q'(j) + 2^(2n-2j)
;Wtedy Q'(j) = Q'(j-1) / 2 + 4^(n-j)*q(j)
;Czyli Q'(j) = ( T(j-1) - 2^(2n-2j) ) / 2 + 2^(2n-2j)*q(j)
;Zatem moge łatwo liczyć poszczególne przejścia
;bo dzielenie przez 2 dużej liczby to rotate w prawo o 1,
;co jest łatwe i proste

;Algorytm dla Q mającego tylko połowę długości X
;polegałby na zdefiniowaniu Q''(j) = Q'(j) / 2^(64*floor((n-j+1)/64))
;Oraz przeliczeniu owych wzrów. Byłaby to optymalizacja wykorzystania pamięci,
;Ale nie byłaby to optymalizacja pod względem czasowym działania programu
;Dlatego u mnie występuje wersja, gdzie alokuję dodatkową pamięć
;Gdyby użytkownik zaalokował w Q 2n bitów to by wystarczyło
;Usunąć sekcję .bss oraz linijki 83-87 oraz pierwsze 6 rozkazów po ".koniec:"

;W algorytmie używam często rejestrów 32-bitowych zamiast 64
;Dzieje sie tak po to, by zmniejszyć liczbe bajtów po kompilacji
;Gdy wiem, że liczba nie przekroczy 2^63-1 to nic to nie zmienia

;musze zpaisac prawdziwe Q
    mov r9, rdi
;musze zwiekszyc n dwukrotnie
    shl edx, 1
;musze zapisac w rdi [rel wieksze_Q]
    lea rdi, [rel wieksze_Q]

;Kod dla n := length(X) i length(Q) = length(X):
    mov r10, rdi
    mov r8, rsi

    mov ebp, edx
    shr ebp, LOG_64

    ;zeruje Q
    xor eax, eax
    mov ecx, ebp
    cld
    rep stosq

    ;ustawiam j na floor(n/2)+1
    mov eax, edx
    shr eax, 1

.petla:
    ;while (j<=n)
    inc eax ;j++
    cmp eax, edx
    ja .koniec

    ;ustawiam rax:=2n-2j
    mov ecx, edx
    sub ecx, eax
    shl ecx, 1

    ;wrzucam do Q 2^(2n-2j)
    bts [r10], ecx

    ;zapisuje rcx w r11
    ;(gdybym od początku robił operacje na r11 to by był 1 bajt więcej)
    mov r11, rcx

    mov ecx, ebp ;dlugosc liczby w slowach
    ;w rdi chce adres T(j-1)
    lea rdi, [r10 + DLUGOSC_SLOWA*rbp - DLUGOSC_SLOWA]
    ;w rsi mam adres R(j-1)
    lea rsi, [r8 + DLUGOSC_SLOWA*rbp - DLUGOSC_SLOWA]
    mov ebx, 1 ;ustawiam 1 w rbx, wyzeruje jeśli okaże się że R>=T
    std ;rejestry beda zmniejszane
    repe cmpsq ;cmp X, Q czyli w obecnym stanie jest to: cmp R, T
    jb .ustawione_R
    ;R>=T
    ;ustawiam R(j)

    mov rdi, r10
    mov rsi, r8
    mov ecx, ebp
    clc
.petla_odejmowanie:
    mov rbx, [rsi]
    sbb rbx, [rdi]
    mov [rsi], rbx
    lea rdi, [rdi + DLUGOSC_SLOWA]
    lea rsi, [rsi + DLUGOSC_SLOWA]
    loop .petla_odejmowanie

    xor ebx, ebx ;ustawiam na 0 by pokazac ze R>=T

.ustawione_R:
    ;ustawiam Q'(j)
    btr [r10], r11 ;cofam z T(j-1) na Q'(j-1)

    ;przesun o 1 bit w prawo Q, czyli podziel na 2
    lea rdi, [r10 + DLUGOSC_SLOWA*rbp - DLUGOSC_SLOWA]
    mov ecx, ebp
    clc
.petla_przesun_bitowo:
    rcr QWORD [rdi], 1
    lea rdi, [rdi - DLUGOSC_SLOWA]
    loop .petla_przesun_bitowo

    cmp ebx, 1
    je .petla
    bts [r10], r11 ;wykonuje tylko dla ebx = 0, czyli dla R>=T
    jmp .petla

.koniec:
;Przywracam do Q wartosc wyliczonego wiekszego Q:
    mov rdi, r9
    mov rsi, r10
    mov ecx, ebp
    shr ecx, 1
    cld
    rep movsq

    przywroc rbp, rbx
    ret
