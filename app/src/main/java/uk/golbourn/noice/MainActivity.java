package uk.golbourn.noice;

import android.content.res.AssetManager;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import uk.golbourn.noice.ui.main.CardConfig;
import uk.golbourn.noice.ui.main.MainFragment;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }

    private static native void initialise(String file1, String file2, String file3, String file4, String file5, String file6, String file7, String file8, AssetManager assetManager);

    private static native void destroy();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initialise(
                CardConfig.cardConfigs[0].getFileName(),
                CardConfig.cardConfigs[1].getFileName(),
                CardConfig.cardConfigs[2].getFileName(),
                CardConfig.cardConfigs[3].getFileName(),
                CardConfig.cardConfigs[4].getFileName(),
                CardConfig.cardConfigs[5].getFileName(),
                CardConfig.cardConfigs[6].getFileName(),
                CardConfig.cardConfigs[7].getFileName(),
                getAssets()
        );
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
        destroy();
    }
}