package wycinanki_macierzowe;

public class Skalar extends TablicaR {
    private Double wartosc;

    public Skalar(double wartosc) {
        this.wartosc = wartosc;
    }

    private Skalar(Skalar skalar) {
        this.wartosc = skalar.wartosc;
    }

    @Override
    public void przemnóż(Wektor wektor) throws BledneParametry {
        throw new BledneParametry("Zbyt wysoki wymiar argumentu!\n");
    }

    @Override
    public void przemnóż(Macierz macierz) throws BledneParametry {
        throw new BledneParametry("Zbyt wysoki wymiar argumentu!\n");
    }

    @Override
    public TablicaR iloczyn(Skalar skalar) {
        return new Skalar(skalar.wartosc * wartosc);
    }

    @Override
    public TablicaR iloczyn(Wektor wektor) {
        Wektor wynik = new Wektor(wektor.liczba_elementów(), wektor.orientacja());
        iloczyn(wektor, wynik);
        return wynik;
    }

    @Override
    public TablicaR iloczyn(Macierz macierz) {
        Macierz wynik = new Macierz(macierz.kształt()[0], macierz.kształt()[1]);
        iloczyn(macierz, wynik);
        return wynik;
    }

    //W przypadku Skalara iloczyn zachowuje się tak samo dla wektora i macierzy
    private void iloczyn(TablicaR tablica, TablicaR wynik) {
        IteratorKolumnowy it = new IteratorKolumnowy(tablica);
        IteratorKolumnowy itWynik = new IteratorKolumnowy(wynik);
        while (it.czyJestNastepny()) {
            itWynik.ustawElement(it.dajElement() * wartosc);
            it.nastepny();
            itWynik.nastepny();
        }
        //ostatni indeks
        itWynik.ustawElement(it.dajElement() * wartosc);
    }


    @Override
    public int wymiar() {
        return 0;
    }

    @Override
    public int liczba_elementów() {
        return 1;
    }

    @Override
    public int[] kształt() {
        return new int[0];
    }

    @Override
    public TablicaR kopia() {
        return new Skalar(wartosc);
    }

    @Override
    public void ustaw(double wartosc, int... idx) {
        super.ustaw(wartosc, idx);
        this.wartosc = wartosc;
    }

    @Override
    public double daj(int... idx) {
        super.daj(idx);
        return wartosc;
    }

    @Override
    public String toString() {
        return Double.toString(wartosc);
    }

    @Override
    public void przypisz(Skalar skalar) {
        wartosc = skalar.wartosc;
    }

    @Override
    public void przypisz(Wektor wektor) throws BledneParametry {
        throw new BledneParametry("Próba przypisania obiektu zbyt wysokiego wymiaru!\n");
    }

    @Override
    public void przypisz(Macierz macierz) throws BledneParametry {
        throw new BledneParametry("Próba przypisania obiektu zbyt wysokiego wymiaru!\n");
    }


    @Override
    public void transponuj() {
        //nic się nie dzieje
    }

    @Override
    public Wycinek wycinek(int... idx) {
        super.wycinek(idx);
        return new Wycinek(new Skalar(this));
    }
}
