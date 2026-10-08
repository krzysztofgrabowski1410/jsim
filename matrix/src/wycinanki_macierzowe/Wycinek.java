package wycinanki_macierzowe;

public class Wycinek extends TablicaR {
    private final TablicaR tablica;

    public Wycinek(TablicaR tablica) {
        this.tablica = tablica;
    }

    @Override
    public TablicaR suma(TablicaR skladnik) throws BledneParametry {
        return tablica.suma(skladnik);
    }

    @Override
    public void dodaj(TablicaR skladnik) throws BledneParametry {
        tablica.dodaj(skladnik);
    }

    @Override
    public void przemnóż(Skalar skalar) {
        tablica.przemnóż(skalar);
    }

    @Override
    public void przemnóż(Wektor wektor) throws BledneParametry {
        tablica.przemnóż(wektor);
    }

    @Override
    public void przemnóż(Macierz macierz) throws BledneParametry {
        tablica.przemnóż(macierz);
    }

    @Override
    public TablicaR iloczyn(Skalar skalar) {
        return tablica.iloczyn(skalar);
    }

    @Override
    public TablicaR iloczyn(Wektor wektor) throws BledneParametry {
        return tablica.iloczyn(wektor);
    }

    @Override
    public TablicaR iloczyn(Macierz macierz) throws BledneParametry {
        return tablica.iloczyn(macierz);
    }

    @Override
    public TablicaR negacja() {
        return tablica.negacja();
    }

    @Override
    public void zaneguj() {
        tablica.zaneguj();
    }

    @Override
    public int wymiar() {
        return tablica.wymiar();
    }

    @Override
    public int liczba_elementów() {
        return tablica.liczba_elementów();
    }

    @Override
    public int[] kształt() {
        return tablica.kształt();
    }

    @Override
    public TablicaR kopia() {
        return tablica.kopia();
    }

    @Override
    public void ustaw(double wartosc, int... idx) {
        tablica.ustaw(wartosc, idx);
    }

    @Override
    public double daj(int... idx) {
        return tablica.daj(idx);
    }

    @Override
    public String toString() {
        return tablica.toString();
    }

    @Override
    public void przypisz(Skalar skalar) {
        tablica.przypisz(skalar);
    }

    @Override
    public void przypisz(Wektor wektor) throws BledneParametry {
        tablica.przypisz(wektor);
    }

    @Override
    public void przypisz(Macierz macierz) throws BledneParametry {
        tablica.przypisz(macierz);
    }

    @Override
    public void transponuj() {
        tablica.transponuj();
    }

    @Override
    public Wycinek wycinek(int... idx) {
        return tablica.wycinek(idx);
    }

    @Override
    public boolean equals(Object obj) {
        return tablica.equals(obj);
    }
}
