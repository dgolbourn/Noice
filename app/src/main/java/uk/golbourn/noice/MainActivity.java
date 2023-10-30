package uk.golbourn.noice;

import android.content.res.AssetManager;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import java.util.Arrays;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }

    private static native void start();

    private static native void stop();

    private static native void lazy_initialise(String[] files, AssetManager assetManager);

    private static native void quick_initialise(String[] files, AssetManager assetManager);


    private static native void destroy();

    private Thread lazyInitialiserThread = new Thread(() -> lazy_initialise(Arrays.stream(CardConfig.cardConfigs).map(CardConfig::getFileName).toArray(String[]::new), getAssets()));

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        start();
        lazyInitialiserThread.start();
        quick_initialise(Arrays.stream(CardConfig.cardConfigs).map(CardConfig::getQuickFileName).toArray(String[]::new), getAssets());
        setContentView(R.layout.activity_main);
        if (savedInstanceState == null) {
            getSupportFragmentManager().beginTransaction()
                    .replace(R.id.container, MainFragment.newInstance())
                    .commitNow();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stop();
        try {
            lazyInitialiserThread.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
        destroy();
    }
}