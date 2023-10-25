package uk.golbourn.noice.ui.main;

public class CardConfig {
    private final int cardId;
    private final int sliderId;

    private final int audioChannel;

    public CardConfig(final int cardId, final int sliderId, final int audioChannel) {
        this.cardId = cardId;
        this.sliderId = sliderId;
        this.audioChannel = audioChannel;
    }

    public int getSliderId() {
        return sliderId;
    }

    public int getCardId() {
        return cardId;
    }

    public int getAudioChannel() {
        return audioChannel;
    }
}
