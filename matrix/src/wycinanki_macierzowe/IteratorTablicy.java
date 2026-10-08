package wycinanki_macierzowe;

import java.util.Arrays;

//Jest to klasa służąca do iteracji po tablicy.
//Rozpakowuje ona Double znajdujące się w obiektach
//do typu podstawowego double; jako element zwraca typ double
public abstract class IteratorTablicy {
    protected final int[] idx;
    protected final TablicaR tablica;

    public abstract void nastepny();

    protected IteratorTablicy(TablicaR tablica) {
        idx = new int[tablica.wymiar()];
        Arrays.fill(idx, 0);
        this.tablica = tablica;
    }

    public double dajElement() {
        if (tablica.czyWTablicy(idx)) {
            return tablica.daj(idx);
        }
        throw new IndeksPozaTablica("Indeks poza tablicą!\n");
    }

    public void ustawElement(double wartosc) {
        if (tablica.czyWTablicy(idx)) {
            tablica.ustaw(wartosc, idx);
            return;
        }
        throw new IndeksPozaTablica("Indeks poza tablicą!\n");
    }

    public void ustawWspolrzedna(int wspolrzedna, int wartosc) {
        if (wspolrzedna > idx.length) {
            throw new IndeksPozaTablica("Za wysoki wymiar argumentu!\n");
        }
        idx[wspolrzedna] = wartosc;
    }

    public int dajWspolrzedna(int wspolrzedna) {
        if (wspolrzedna > idx.length) {
            throw new IndeksPozaTablica("Za wysoki wymiar argumentu!\n");
        }
        return idx[wspolrzedna];
    }

    public void nastepnyPetla() {
        try {
            nastepny();
        } catch (IndeksPozaTablica wyjatek) {
            if (idx.length != 0) {
                Arrays.fill(idx, 0);
            }
        }
    }

    public boolean czyJestNastepny() {
        for (int i = 0; i < tablica.wymiar(); i++) {
            if (idx[i] + 1 < tablica.kształt()[i]) {
                return true;
            }
        }
        return false;
    }

}
