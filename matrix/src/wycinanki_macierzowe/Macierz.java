package wycinanki_macierzowe;

import java.util.Arrays;

public class Macierz extends TablicaR {

    private Double[][] tablica;

    public Macierz(int wiersze, int kolumny) {
        if (wiersze == 0 || kolumny == 0) {
            throw new PusteParametry("Macierz nie może być pusta!\n");
        }
        tablica = new Double[wiersze][kolumny];
        for (int i = 0; i < wiersze; i++) {
            Double[] tmp = new Double[kolumny];
            Arrays.fill(tmp, (double) 0);
            tablica[i] = tmp;
        }
    }

    public Macierz(Skalar s, int wielkosc) {
        if (wielkosc == 0) {
            throw new PusteParametry("Macierz nie może być pusta!\n");
        }
        tablica = new Double[wielkosc][wielkosc];
        for (int i = 0; i < wielkosc; i++) {
            Double[] tmp = new Double[wielkosc];
            Arrays.fill(tmp, (double)0);
            tmp[i] = s.daj();
            tablica[i] = tmp;
        }
    }

    //kopiuje
    public Macierz(Double[][] tablica) {
        if (tablica.length == 0) {
            throw new PusteParametry("Macierz nie może być pusta!\n");
        }
        this.tablica = new Double[tablica.length][tablica[0].length];
        for (int i = 0; i < tablica.length; i++) {
            this.tablica[i] = TablicaR.glebokaKopiaTablicy(tablica[i]);
        }
    }

    public Macierz(double[][] tablica) {
        if (tablica.length == 0) {
            throw new PusteParametry("Macierz nie może być pusta!\n");
        }
        this.tablica = new Double[tablica.length][tablica[0].length];
        for (int i = 0; i < tablica.length; i++) {
            this.tablica[i] = TablicaR.glebokaKopiaTablicy(tablica[i]);
        }
    }

    private Macierz(Macierz macierz, int startY, int startX, int koniecY, int koniecX) {
        tablica = new Double[koniecY - startY + 1][koniecX - startX + 1];
        for (int i = 0; i < koniecY - startY + 1; i++) {
            tablica[i] = Arrays.copyOfRange(macierz.tablica[i + startY], startX, koniecX + 1);
        }
    }

    //Tutaj dodaje macierz do macierzy
    @Override
    protected void sumaPom(TablicaR skladnik, TablicaR wynik) throws BledneParametry {
        if (!Arrays.equals(kształt(), skladnik.kształt())) {
            throw new BledneParametry("Dodawanie macierzy różnych kształtów!\n");
        }
        super.sumaPom(skladnik, wynik);
    }

    @Override
    public void przemnóż(Wektor wektor) throws BledneParametry {
        throw new BledneParametry("Wynik operacji nie byłby macierzą!\n");
    }

    @Override
    public void przemnóż(Macierz macierz) throws BledneParametry {
        if (macierz.kształt()[0] != kształt()[1]) {
            throw new BledneParametry("Kształt macierzy jest błędny!\n");
        }
        this.przypisz(iloczyn(macierz));
    }

    @Override
    public TablicaR iloczyn(Skalar skalar) {
        return skalar.iloczyn(this);
    }

    @Override
    public TablicaR iloczyn(Wektor wektor) throws BledneParametry {
        return wektor.iloczyn(this);
    }

    @Override
    public Macierz iloczyn(Macierz macierz) throws BledneParametry {
        if (kształt()[1] != macierz.kształt()[0]) {
            throw new BledneParametry("Kształt macierzy jest błędny!\n");
        }
        //Mnożenie macierzy:
        Macierz wynik = new Macierz(kształt()[0], macierz.kształt()[1]); //zmienic constructor
        IteratorWierszowy it1 = new IteratorWierszowy(this);
        IteratorKolumnowy it2 = new IteratorKolumnowy(macierz);
        IteratorKolumnowy itWynik = new IteratorKolumnowy(wynik);
        boolean ostatni = true;
        while (itWynik.czyJestNastepny() || ostatni) {
            if (itWynik.czyJestNastepny()) {
                ostatni = false;
            }
            it1.ustawWspolrzedna(0, itWynik.dajWspolrzedna(0));
            it1.ustawWspolrzedna(1, 0);
            it2.ustawWspolrzedna(1, itWynik.dajWspolrzedna(1));
            it2.ustawWspolrzedna(0, 0);
            double wartosc = 0;
            int powtorzenie = 0;
            while (it1.czyJestNastepny() && powtorzenie < macierz.kształt()[0] - 1) {
                wartosc += it1.dajElement() * it2.dajElement();
                it1.nastepny();
                it2.nastepny();
                powtorzenie++;
            }
            wartosc += it1.dajElement() * it2.dajElement();
            itWynik.ustawElement(wartosc);
            if (itWynik.czyJestNastepny()) {
                itWynik.nastepny();
            }
            ostatni = !itWynik.czyJestNastepny() && !ostatni;
        }
        return wynik;
    }

    @Override
    public int wymiar() {
        return 2;
    }

    @Override
    public int liczba_elementów() {
        return tablica.length * tablica[0].length;
    }

    @Override
    public int[] kształt() {
        int[] ksztalt = new int[2];
        ksztalt[0] = tablica.length;
        ksztalt[1] = tablica[0].length;
        return ksztalt;
    }

    @Override
    public TablicaR kopia() {
        return new Macierz(tablica);
    }

    @Override
    public void ustaw(double wartosc, int... idx) {
        super.ustaw(wartosc, idx);
        tablica[idx[0]][idx[1]] = wartosc;
    }

    @Override
    public double daj(int... idx) {
        super.daj(idx);
        return tablica[idx[0]][idx[1]];
    }

    @Override
    public String toString() {
        return Arrays.deepToString(tablica);
    }

    @Override
    public void przypisz(Skalar skalar) {
        IteratorWierszowy it = new IteratorWierszowy(this);
        while (it.czyJestNastepny()) {
            it.ustawElement(skalar.daj());
            it.nastepny();
        }
        it.ustawElement(skalar.daj());
    }

    @Override
    public void przypisz(Wektor wektor) throws BledneParametry {
        IteratorTablicy it1;
        if (wektor.orientacja()) {
            it1 = new IteratorWierszowy(this);
            if (wektor.liczba_elementów() != kształt()[1]) {
                throw new BledneParametry("Kształt wektora jest błędny!\n");
            }
        } else {
            it1 = new IteratorKolumnowy(this);
            if (wektor.liczba_elementów() != kształt()[0]) {
                throw new BledneParametry("Kształt wektora jest błędny!\n");
            }
        }
        IteratorWierszowy it2 = new IteratorWierszowy(wektor);
        while (it1.czyJestNastepny()) {
            it1.ustawElement(it2.dajElement());
            it1.nastepny();
            it2.nastepnyPetla();
        }
        it1.ustawElement(it2.dajElement());
    }

    @Override
    public void przypisz(Macierz macierz) throws BledneParametry {
        if (this.kształt()[0] != macierz.kształt()[0] || this.kształt()[1] != macierz.kształt()[1]) {
            throw new BledneParametry("Próba przypisania macierzy o różnym kształcie!\n");
        }
        this.tablica = new Double[macierz.kształt()[0]][macierz.kształt()[1]];
        for (int i = 0; i < macierz.kształt()[0]; i++) {
            this.tablica[i] = TablicaR.glebokaKopiaTablicy(macierz.tablica[i]);
        }
    }

    @Override
    public void transponuj() {
        IteratorKolumnowy it1 = new IteratorKolumnowy(this);
        IteratorWierszowy it2 = new IteratorWierszowy(this);
        double tymczasowe;
        while (it1.czyJestNastepny()) {
            tymczasowe = it1.dajElement();
            it1.ustawElement(it2.dajElement());
            it2.ustawElement(tymczasowe);
            it1.nastepny();
            it2.nastepny();
        }
        tymczasowe = it1.dajElement();
        it1.ustawElement(it2.dajElement());
        it2.ustawElement(tymczasowe);
    }

    @Override
    protected boolean czyTeSameWymiary(Wektor wektor) {
        if (!wektor.orientacja()) {
            return kształt()[0] == wektor.liczba_elementów();
        }
        return kształt()[1] == wektor.liczba_elementów();
    }

    @Override
    public Wycinek wycinek(int... idx) {
        super.wycinek(idx);
        return new Wycinek(new Macierz(this, idx[0], idx[2], idx[1], idx[3]));
    }
}
