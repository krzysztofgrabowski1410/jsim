package wycinanki_macierzowe;

public abstract class TablicaR {

    public TablicaR suma(TablicaR skladnik) throws BledneParametry {
        TablicaR wynik;
        if (this.wymiar() > skladnik.wymiar()) {
            wynik = this.kopia();
            skladnik.sumaPom(this, wynik);
        } else {
            wynik = skladnik.kopia();
            this.sumaPom(skladnik, wynik);
        }
        return wynik;
    }

    public void dodaj(TablicaR skladnik) throws BledneParametry {
        if (this.wymiar() < skladnik.wymiar()) {
            throw new BledneParametry("Wymiar prawego składnika jest mniejszy od wymiaru lewego!\n");
        }
        skladnik.sumaPom(this, this);
    }

    //wyjątki będą zaimplementowane w podklasach
    //tutaj wiem że dim(this) <= dim(skladnik)
    //dodawanie jest przemienne wiec mogłem zamienić miejscami elementy
    protected void sumaPom(TablicaR skladnik, TablicaR wynik) throws BledneParametry {
        IteratorKolumnowy it1 = new IteratorKolumnowy(this);
        IteratorKolumnowy it2 = new IteratorKolumnowy(skladnik);
        IteratorKolumnowy itWynik = new IteratorKolumnowy(wynik);
        while (it2.czyJestNastepny()) {
            itWynik.ustawElement(it1.dajElement() + it2.dajElement());
            it1.nastepnyPetla();
            it2.nastepny();
            itWynik.nastepny();
        }
        itWynik.ustawElement(it1.dajElement() + it2.dajElement());
    }

    public void przemnóż(Skalar skalar) {
        IteratorWierszowy it = new IteratorWierszowy(this);
        while (it.czyJestNastepny()) {
            it.ustawElement(it.dajElement() * skalar.daj());
            it.nastepny();
        }
        it.ustawElement(it.dajElement() * skalar.daj());
    }

    public abstract void przemnóż(Wektor wektor) throws BledneParametry;

    public abstract void przemnóż(Macierz macierz) throws BledneParametry;

    public abstract TablicaR iloczyn(Skalar skalar);

    public abstract TablicaR iloczyn(Wektor wektor) throws BledneParametry;

    public abstract TablicaR iloczyn(Macierz macierz) throws BledneParametry;

    public TablicaR negacja() {
        TablicaR wynik = this.kopia();
        IteratorKolumnowy it = new IteratorKolumnowy(this);
        IteratorKolumnowy itWynik = new IteratorKolumnowy(wynik);
        while (it.czyJestNastepny()) {
            itWynik.ustawElement(it.dajElement() * (-1));
            itWynik.nastepny();
            it.nastepny();
        }
        itWynik.ustawElement(it.dajElement() * (-1));
        return wynik;
    }

    public void zaneguj() {
        IteratorKolumnowy it = new IteratorKolumnowy(this);
        while (it.czyJestNastepny()) {
            it.ustawElement(it.dajElement() * (-1));
            it.nastepny();
        }
        it.ustawElement(it.dajElement() * (-1));
    }

    public abstract int wymiar();

    public abstract int liczba_elementów();

    public abstract int[] kształt();

    public abstract TablicaR kopia();

    public void ustaw(double wartosc, int... idx) {
        if (idx.length > wymiar()) {
            throw new IndeksPozaTablica("Podano zbyt wiele liczb!\n");
        }
        if (idx.length < wymiar()) {
            throw new IndeksPozaTablica("Podano zbyt mało liczb!\n");
        }
    }

    public double daj(int... idx) {
        if (idx.length > wymiar()) {
            throw new IndeksPozaTablica("Podano zbyt wiele liczb!\n");
        }
        if (idx.length < wymiar()) {
            throw new IndeksPozaTablica("Podano zbyt mało liczb!\n");
        }
        if (!czyWTablicy(idx)) {
            throw new IndeksPozaTablica("Próba wyjścia poza tablicę!\n");
        }
        return 0;
    }

    @Override
    public abstract String toString();

    public abstract void przypisz(Skalar skalar);

    public abstract void przypisz(Wektor wektor) throws BledneParametry;

    public abstract void przypisz(Macierz macierz) throws BledneParametry;

    public abstract void transponuj();

    protected boolean czyWTablicy(int[] idx) {
        for (int i = 0; i < wymiar(); i++) {
            if (idx[i] > kształt()[i]) {
                return false;
            }
        }
        return true;
    }

    //kopiuje tablicę Doubli
    protected static Double[] glebokaKopiaTablicy(Double[] tablica) {
        Double[] wynik = new Double[tablica.length];
        for (int i = 0; i < tablica.length; i++) {
            double tmp = tablica[i];
            wynik[i] = tmp;
        }
        return wynik;
    }

    //kopiuje tablicę Doubli
    protected static Double[] glebokaKopiaTablicy(double[] tablica) {
        Double[] wynik = new Double[tablica.length];
        for (int i = 0; i < tablica.length; i++) {
            double tmp = tablica[i];
            wynik[i] = tmp;
        }
        return wynik;
    }

    protected boolean czyTeSameWymiary(Wektor wektor) {
        return true;
    }

    public Wycinek wycinek(int... idx) {
        //ta funkcja tylko sprawdza poprawność parametrów, dlatego zwraca null
        if (idx.length > 2 * wymiar()) {
            throw new IndeksPozaTablica("Podano zbyt wiele liczb!\n");
        }
        if (idx.length < 2 * wymiar()) {
            throw new IndeksPozaTablica("Podano zbyt mało liczb!\n");
        }
        for (int i = 0; i < idx.length / 2; i += 2) {
            if (idx[i] > idx[i + 1] || idx[i + 1] >= kształt()[i] || idx[i] < 0) {
                throw new IndeksPozaTablica("Podano błędne liczby!\n");
            }
        }
        return null;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() == obj.getClass()) {
            TablicaR obj2 = (TablicaR) obj;
            IteratorWierszowy it1 = new IteratorWierszowy(obj2);
            IteratorWierszowy it2 = new IteratorWierszowy(this);
            while (it1.czyJestNastepny() && it2.czyJestNastepny()) {
                if (it1.dajElement() != it2.dajElement()) {
                    return false;
                }
                it1.nastepny();
                it2.nastepny();
            }
            //obiekty są różnych rozmiarów:
            if (it1.czyJestNastepny() || it2.czyJestNastepny()) {
                return false;
            }
            return it1.dajElement() == it2.dajElement();
        }
        if (obj.getClass() == Wycinek.class) {
            return obj.equals(this);
        }
        return false;
    }
}
