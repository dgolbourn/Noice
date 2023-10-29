package uk.golbourn.noice;

public class CardConfig {

    public static final CardConfig[] cardConfigs = {
            new CardConfig(R.id.card_1, R.id.slider_1, 0, "quick_brook.mp3", "brook.mp3"),
            new CardConfig(R.id.card_2, R.id.slider_2, 1, "quick_birds.mp3", "birds.mp3"),
            new CardConfig(R.id.card_3, R.id.slider_3, 2, "quick_city.mp3", "city.mp3"),
            new CardConfig(R.id.card_4, R.id.slider_4, 3, "quick_rain.mp3", "rain.mp3"),
            new CardConfig(R.id.card_5, R.id.slider_5, 4, "quick_waves.mp3", "waves.mp3"),
            new CardConfig(R.id.card_6, R.id.slider_6, 5, "quick_radio.mp3", "radio.mp3"),
            new CardConfig(R.id.card_7, R.id.slider_7, 6, "quick_fan.mp3", "fan.mp3"),
            new CardConfig(R.id.card_8, R.id.slider_8, 7, "quick_people.mp3", "people.mp3")
    };

    private final int cardId;
    private final int sliderId;
    private final int audioChannel;
    private final String fileName;
    private final String quickFileName;

    public CardConfig(final int cardId, final int sliderId, final int audioChannel, final String quickFileName, final String fileName) {
        this.cardId = cardId;
        this.sliderId = sliderId;
        this.audioChannel = audioChannel;
        this.fileName = fileName;
        this.quickFileName = quickFileName;
    }

    public int getSliderId() {
        return sliderId;
    }

    public String getQuickFileName() {
        return quickFileName;
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
