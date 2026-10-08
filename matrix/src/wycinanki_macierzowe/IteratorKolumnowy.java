package wycinanki_macierzowe;

import java.util.Arrays;

public class IteratorKolumnowy extends IteratorTablicy {

    protected IteratorKolumnowy(TablicaR tablica) {
        super(tablica);
    }

    @Override
    public void nastepny() {
        for (int i = 0; i < idx.length; i++) {
            int dlugosc = tablica.kształt()[i];
            if (idx[i] + 1 < dlugosc) {
                Arrays.fill(idx, 0, i, 0);
                idx[i]++;
                return;
            }
        }
        Arrays.fill(idx, 0);
        throw new IndeksPozaTablica("Indeks poza tablicą!\n");
    }

}
