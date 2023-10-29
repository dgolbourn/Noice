package uk.golbourn.noice.ui.main;

import uk.golbourn.noice.R;

public class CardConfig {

    public static final CardConfig[] cardConfigs = {
            new CardConfig(R.id.card_1, R.id.slider_1, 0, "brook.mp3"),
            new CardConfig(R.id.card_2, R.id.slider_2, 1, "birds.mp3"),
            new CardConfig(R.id.card_3, R.id.slider_3, 2, "city.mp3"),
            new CardConfig(R.id.card_4, R.id.slider_4, 3, "rain.mp3"),
            new CardConfig(R.id.card_5, R.id.slider_5, 4, "waves.mp3"),
            new CardConfig(R.id.card_6, R.id.slider_6, 5, "radio.mp3"),
            new CardConfig(R.id.card_7, R.id.slider_7, 6, "fan.mp3"),
            new CardConfig(R.id.card_8, R.id.slider_8, 7, "people.mp3")
    };

    private final int cardId;
    private final int sliderId;

    private final int audioChannel;

    private final String fileName;

    public CardConfig(final int cardId, final int sliderId, final int audioChannel, final String fileName) {
        this.cardId = cardId;
        this.sliderId = sliderId;
        this.audioChannel = audioChannel;
        this.fileName = fileName;
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

    public String getFileName() {
        return fileName;
    }
}
