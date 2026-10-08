package wycinanki_macierzowe;

import java.util.Arrays;

public class IteratorWierszowy extends IteratorTablicy {

    public IteratorWierszowy(TablicaR tablica) {
        super(tablica);
    }

    @Override
    public void nastepny() {
        for (int i = idx.length - 1; i >= 0; i--) {
            int dlugosc = tablica.kształt()[i];
            if (idx[i] + 1 < dlugosc) {
                Arrays.fill(idx, i + 1, idx.length, 0);
                idx[i]++;
                return;
            }
        }
        Arrays.fill(idx, 0);
        throw new IndeksPozaTablica("Indeks poza tablicą!\n");
    }

}
