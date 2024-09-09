package uk.golbourn.noice;

public record CardConfig(int cardId, int sliderId, int audioChannel, String fileName) {
    public static final CardConfig[] cardConfigs = {
            new CardConfig(R.id.card_1, R.id.slider_1, 0, "calm.mp3"),
            new CardConfig(R.id.card_2, R.id.slider_2, 1, "forest.mp3"),
            new CardConfig(R.id.card_3, R.id.slider_3, 2, "train.mp3"),
            new CardConfig(R.id.card_4, R.id.slider_4, 3, "rain.mp3"),
            new CardConfig(R.id.card_5, R.id.slider_5, 4, "waves.mp3"),
            new CardConfig(R.id.card_6, R.id.slider_6, 5, "fire.mp3"),
            new CardConfig(R.id.card_7, R.id.slider_7, 6, "car.mp3"),
            new CardConfig(R.id.card_8, R.id.slider_8, 7, "wind.mp3")
    };
}
