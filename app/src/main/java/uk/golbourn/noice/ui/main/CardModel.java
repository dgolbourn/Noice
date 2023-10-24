package uk.golbourn.noice.ui.main;

public class CardModel {
    private final int cardId;
    private final int sliderId;

    public CardModel(final int cardId, final int sliderId) {
        this.cardId = cardId;
        this.sliderId = sliderId;
    }

    public int getSliderId() {
        return sliderId;
    }

    public int getCardId() {
        return cardId;
    }
}
