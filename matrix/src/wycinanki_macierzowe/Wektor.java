package wycinanki_macierzowe;

import java.util.Arrays;

public class Wektor extends TablicaR {

    private Double[] tablica;
    //false - pozioma, true - pionowa
    private boolean orientacja;

    public Wektor(double[] tablica, boolean orientacja) {
        if (tablica.length == 0) {
            throw new PusteParametry("Wektor nie może być pusty!\n");
        }
        this.tablica = TablicaR.glebokaKopiaTablicy(tablica);
        this.orientacja = orientacja;
    }

    //kopia wektora
    public Wektor(Double[] tablica, boolean orientacja) {
        if (tablica.length == 0) {
            throw new PusteParametry("Wektor nie może być pusty!\n");
        }
        this.tablica = TablicaR.glebokaKopiaTablicy(tablica);
        this.orientacja = orientacja;
    }

    //wyzerowany wektor
    public Wektor(int size, boolean orientacja) {
        if (size == 0) {
            throw new PusteParametry("Wektor nie może być pusty!\n");
        }
        tablica = new Double[size];
        Arrays.fill(tablica, (double)0);
        this.orientacja = orientacja;
    }

    private Wektor(Wektor wektor, int start, int koniec) {
        tablica = Arrays.copyOfRange(wektor.tablica, start, koniec + 1);
        orientacja = wektor.orientacja;
    }

    @Override
    protected void sumaPom(TablicaR skladnik, TablicaR wynik) throws BledneParametry {
        if (!skladnik.czyTeSameWymiary(this)) {
            throw new BledneParametry("Błędny kształt obiektu!\n");
        }
        IteratorTablicy it1;
        IteratorTablicy it2;
        IteratorTablicy itWynik;
        if (!orientacja) {
            it1 = new IteratorKolumnowy(this);
            it2 = new IteratorKolumnowy(skladnik);
            itWynik = new IteratorKolumnowy(wynik);
        } else {
            it1 = new IteratorWierszowy(this);
            it2 = new IteratorWierszowy(skladnik);
            itWynik = new IteratorWierszowy(wynik);
        }
        while (it2.czyJestNastepny()) {
            itWynik.ustawElement(it1.dajElement() + it2.dajElement());
            it1.nastepnyPetla();
            it2.nastepny();
            itWynik.nastepny();
        }
        itWynik.ustawElement(it1.dajElement() + it2.dajElement());
    }

    private Skalar iloczynSkalarny(Wektor wektor) {
        Skalar wynik = new Skalar(0.0);
        IteratorWierszowy it1 = new IteratorWierszowy(this);
        IteratorWierszowy it2 = new IteratorWierszowy(wektor);
        while (it1.czyJestNastepny()) {
            wynik.ustaw(wynik.daj() + it1.dajElement() * it2.dajElement());
            it1.nastepny();
            it2.nastepny();
        }
        wynik.ustaw(wynik.daj() + it1.dajElement() * it2.dajElement());
        return wynik;
    }

    @Override
    public void przemnóż(Wektor wektor) throws BledneParametry {
        if (orientacja != wektor.orientacja) {
            throw new BledneParametry("Wektor ma błędną orientację!\n");
        } else if (liczba_elementów() != wektor.liczba_elementów()) {
            throw new BledneParametry("Błędny kształt wektora!\n");
        }
        przypisz(iloczynSkalarny(wektor));
    }

    @Override
    public void przemnóż(Macierz macierz) throws BledneParametry {
        throw new BledneParametry("Zbyt wysoki wymiar argumentu!\n");
    }

    @Override
    public TablicaR iloczyn(Skalar skalar) {
        return skalar.iloczyn(this);
    }

    @Override
    public TablicaR iloczyn(Wektor wektor) throws BledneParametry {
        if (wektor.liczba_elementów() != liczba_elementów()) {
            throw new BledneParametry("Błędny kształt wektora!\n");
        }
        if (orientacja == wektor.orientacja()) {
            return iloczynSkalarny(wektor);
        } else if (orientacja) {
            Macierz lewa = new Macierz(1, liczba_elementów());
            Macierz prawa = new Macierz(liczba_elementów(), 1);
            lewa.przypisz(this);
            prawa.przypisz(wektor);
            return lewa.iloczyn(prawa);
        } else {
            Macierz lewa = new Macierz(liczba_elementów(),1);
            Macierz prawa = new Macierz(1, liczba_elementów());
            lewa.przypisz(this);
            prawa.przypisz(wektor);
            return lewa.iloczyn(prawa);
        }
    }

    @Override
    public Wektor iloczyn(Macierz macierz) throws BledneParametry {
        if (orientacja && macierz.kształt()[0] != liczba_elementów() ||
                (!orientacja && macierz.kształt()[1] != liczba_elementów())) {
            throw new BledneParametry("Błędny kształt macierzy!\n");
        }
        Wektor wynik;
        IteratorTablicy it1;
        if (!orientacja) {
            wynik = new Wektor(macierz.kształt()[0], orientacja);
            it1 = new IteratorWierszowy(macierz);
        } else {
            wynik = new Wektor(macierz.kształt()[1], orientacja);
            it1 = new IteratorKolumnowy(macierz);
        }
        IteratorWierszowy itWynik = new IteratorWierszowy(wynik);
        boolean ostatni = true;
        while (itWynik.czyJestNastepny() || ostatni) {
            if (itWynik.czyJestNastepny()) {
                ostatni = false;
            }
            Wektor czescMacierzy = new Wektor(liczba_elementów(), !orientacja);
            IteratorWierszowy it2 = new IteratorWierszowy(czescMacierzy);
            for (int i = 0; i < liczba_elementów() - 1; i++) {
                it2.ustawElement(it1.dajElement());
                it1.nastepny();
                it2.nastepny();
            }
            it2.ustawElement(it1.dajElement());
            if (it1.czyJestNastepny()) {
                it1.nastepny();
            }
            if (!orientacja) {
                itWynik.ustawElement(czescMacierzy.iloczyn(this).daj(0, 0));
            } else {
                itWynik.ustawElement(iloczyn(czescMacierzy).daj(0, 0));
            }
            if (itWynik.czyJestNastepny()) {
                itWynik.nastepny();
            }
            ostatni = !itWynik.czyJestNastepny() && !ostatni;
        }
        return wynik;
    }

    @Override
    public int wymiar() {
        return 1;
    }

    @Override
    public int liczba_elementów() {
        return tablica.length;
    }

    @Override
    public int[] kształt() {
        return new int[]{tablica.length};
    }

    @Override
    public TablicaR kopia() {
        return new Wektor(tablica, orientacja);
    }

    @Override
    public void ustaw(double wartosc, int... idx) {
        super.ustaw(wartosc, idx);
        if (czyWTablicy(idx)) {
            tablica[idx[0]] = wartosc;
        } else {
            throw new IndeksPozaTablica("Próba wyjścia poza tablicę.\n");
        }
    }

    @Override
    public double daj(int... idx) {
        super.daj(idx);
        return tablica[idx[0]];
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        if (orientacja) {
            builder.append("Orientacja: pozioma; Wektor: ");
        } else {
            builder.append("Orientacja: pionowa; Wektor: ");
        }
        builder.append(Arrays.deepToString(tablica));
        builder.append("\n");
        return builder.toString();
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
        if (wektor.liczba_elementów() != this.liczba_elementów()) {
            throw new BledneParametry("Próba przypisania wektora o różnej liczbie elementów!\n");
        }
        this.orientacja = wektor.orientacja();
        this.tablica = glebokaKopiaTablicy(wektor.tablica);
    }

    @Override
    public void przypisz(Macierz macierz) throws BledneParametry {
        throw new BledneParametry("Próba przypisania obiektu zbyt wysokiego wymiaru!\n");
    }

    @Override
    public void transponuj() {
        orientacja = !orientacja;
    }

    public boolean orientacja() {
        return orientacja;
    }

    @Override
    protected boolean czyTeSameWymiary(Wektor wektor) {
        return orientacja == wektor.orientacja && liczba_elementów() == wektor.liczba_elementów();
    }

    @Override
    public Wycinek wycinek(int... idx) {
        super.wycinek(idx);
        return new Wycinek(new Wektor(this, idx[0], idx[1]));
    }
}
