# **Raport Laboratorium 3**

### **Prowadzący:** Mgr inż. Przemysław Świercz
### **Oddaje:** Tomasz Celmer 218242

#### **Przedmiot:** Organizacja i architektura komputerów
---
# Cel laboratorium

Celem laboratorium była implementacja programu mnożącego dwie całkowite liczby. Program miał za zadanie spytać użytkownika o dwie liczby w notacji hexadecymalnej, a następnie pokazać wynik mnożenia tych dwóch liczb.

---

# Założenia programu: 
* liczby powinny być wpisywane w systemie szestanskowym
* wynik powinny być wyświetlnay w systemie szestanstkowym
* limit znaków to 200
* wykorzystanie algorytmu mnożenia dużych liczb np. mnożenie przez części
* poprawne działanie w architekturze 32-bitowej na platformie linux 
---

# Opis implementacji

1. Pierwszym krokiem było wprowadzenie zmiennych dla czytelności kodu

```assembly
// rozmiar liczby 
size = 200
// rozmiar wyniku
size_double = 401

// ciag formatujacy

.ciagFormatujacyDoWczytywania:
        .string "%s"
.msgWynik:
        .string "\nwynik w ASCII:"
```
---
2. Następnie dokonywane zrzucenia rejestrów na stos

```assembly

pushl   %esp
pushl   %ebp
movl    %esp, %ebp
pushl   %edi
pushl   %ebx
pushl   %ecx
```
---

3. Rezerwacja pamięci na stosie, zgodnie z założeniami zadania na ilość znaków. Dla każdej liczby jest rezerwowane po 200 + 1 bajtów na znak '\0' na końcu strniga, natomiast dla wyniku 400 + 1 znak '0' na końcu stringa, więc łącznie rezerwowane jest 201+201+401+bufor = 876 bajtów  

```assembly

    subl    $876, %esp
```
---

4. W celu łatwiejszego rozpoznania niezapisanych wartości, tworzona jest pętla wypełniającą dwie liczby wartością -1. Dlaczego to jest robione? Przykład: 200-1-1-1-1 == 200, bez tego kroku ciężko byłoby rozpoznać wartość 2000000 = 20, 200, 2000 ? Rejestr %ECX używany jest jak licznik, wpisana jest do niego wartość początkowa zero, dzięki niemu możliwa jest iteracja po pętli aż do napotkania wartości rozmiaru size_double. Następnie do rejestru %eax wpisywany jest adres początekuliczby1 i następuje wypełnianie wartością -1 dwóch liczb.

```assembly

        movl    $0, %ecx
.Petla_InicjalizujLiczby:
        cmpl    $size_double, %ecx      # porównaj. Jeśli większe, wyjdź z pętli
        jg      .WczytajLiczby
        
        leal    -479(%ebp), %eax        # 1) wpisz adres początku liczby1 do %eax 
        addl    %ecx, %eax              # 2) zwiększ pozycję (adres w %eax) o wartość licznika 
        movb    $-1, (%eax)             # 3) zapisz -1 pod tym adresem aż do końca drugiej liczby
        
        addl    $1, %ecx                # zwiększ %ecx o jeden == i++ 
        jmp     .Petla_InicjalizujLiczby # i wróć na początek pętli

```
---

5. Implementacja wczytywania liczb

```assembly

.WczytajLiczby:

        // wczytaj drugą liczbę

        subl    $8, %esp
        leal    -278(%ebp), %eax
        pushl   %eax
        pushl   $.ciagFormatujacyDoWczytywania
        call    scanf
        addl    $16, %esp

        // wczytaj pierwszą liczbę

        subl    $8, %esp
        leal    -479(%ebp), %eax
        pushl   %eax
        pushl   $.ciagFormatujacyDoWczytywania
        call    scanf
        addl    $16, %esp

```
---

6. Implementacja pętli zamieniającą kody ASCII na wartości liczbowe, rejestr %esi używany jest jako licznik, wpisujemy do niego wartość 0 i iterujemy aż do wartości rozmiaru wyniku (size_double)

```assembly

.WczytajLiczby:

        // wczytaj drugą liczbę

        subl    $8, %esp
        leal    -278(%ebp), %eax
        pushl   %eax
        pushl   $.ciagFormatujacyDoWczytywania
        call    scanf
        addl    $16, %esp

        // wczytaj pierwszą liczbę

        subl    $8, %esp
        leal    -479(%ebp), %eax
        pushl   %eax
        pushl   $.ciagFormatujacyDoWczytywania
        call    scanf
        addl    $16, %esp

```
---

7. Implementacja pętli zamieniającą kody ASCII na wartości liczbowe, rejestr %esi używany jest jako licznik, wpisujemy do niego wartość 0 i iterujemy aż do wartości rozmiaru wyniku (size_double). W pętli sprawdzane jest znak jest literą A,B,C .., jeżeli tak to zgodnia z tabelą ASCII odejmowana jest wartość 55, sprawdzane jest również czy znak jest cyfrą, jeżeli tak to zgodnie z tabelą ASCII odejmowana jest wartość 47.

```assembly
        movl    $0, %esi
.Petla_ZamienAsciiNaWartosci:

        cmpl    $size_double, %esi              # porównaj. Jeśli większe, wyjdź z pętli
        jg      .ZnajdzPoczatkiLiczb

        leal    -479(%ebp), %eax
        addl    %esi, %eax
        movzbl  (%eax), %eax

        // Sprawdź czy znak > 64 -> czy jest literą A,B,C,...
        // Jeśli znak ma wartość <= 64, przejdź do sprawdzania czy jest cyfrą
        cmpb    $64, %al
        jle     .SprawdzCzyJestCyfra

        // Jeśli jest literą, odejmij 55 (zgodnie z tabelą ASCII)
        // -> zamień literę na wartość liczbową: A->10, B->11, itd.
        
        // pobierz wartość aktualnego znaku

        leal    -479(%ebp), %eax
        addl    %esi, %eax
        movzbl  (%eax), %eax

        // odejmij 55 i zapisz z powrotem na miejsce tego znaku

        subl    $55, %eax
        movl    %eax, %edx
        leal    -479(%ebp), %eax
        addl    %esi, %eax
        movb    %dl, (%eax)

.SprawdzCzyJestCyfra:
        leal    -479(%ebp), %eax
        addl    %esi, %eax
        movzbl  (%eax), %eax

        // Sprawdź czy znak > 47 -> czy jest cyfrą 0, 1, ..., 9
        // Jeśli znak ma wartość <= 47, przejdź na koniec pętli

        cmpb    $47, %al
        jle     .koniecPętli_ZamienAsciiNaWartosc

        // Jeśli jest cyfrą, odejmij 48 (zgodnie z tabelą ASCII)
        // -> zamień cyfrę ASCII na wartość liczbową

        leal    -479(%ebp), %eax
        addl    %esi, %eax
        movzbl  (%eax), %eax
        subl    $48, %eax
        movl    %eax, %edx
        leal    -479(%ebp), %eax
        addl    %esi, %eax
        movb    %dl, (%eax)

.koniecPętli_ZamienAsciiNaWartosc:
        addl    $1, %esi
        jmp     .Petla_ZamienAsciiNaWartosci


```
---





# Napotkane problemy

* Po wykonanym mnożeniu liczb, dopisywane jest na końcu zbędne zero


# Wnioski


# Użyte materiały

* [Dokumentacja GNU Assembler](https://sourceware.org/binutils/docs/as/)

* [Linux System Call Table](https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md)
