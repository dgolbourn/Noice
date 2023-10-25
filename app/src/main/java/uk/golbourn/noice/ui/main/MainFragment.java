package uk.golbourn.noice.ui.main;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.card.MaterialCardView;
import com.google.android.material.slider.Slider;

import uk.golbourn.noice.R;

public class MainFragment extends Fragment {

    private final CardConfig[] cardConfigs = {
            new CardConfig(R.id.card_1, R.id.slider_1, 1),
            new CardConfig(R.id.card_2, R.id.slider_2, 2),
            new CardConfig(R.id.card_3, R.id.slider_3, 3),
            new CardConfig(R.id.card_4, R.id.slider_4, 4),
            new CardConfig(R.id.card_5, R.id.slider_5, 5),
            new CardConfig(R.id.card_6, R.id.slider_6, 6),
            new CardConfig(R.id.card_7, R.id.slider_7, 7),
            new CardConfig(R.id.card_8, R.id.slider_8, 8)
    };
    private MainViewModel viewModel;
    private AudioService audioService;
    private ServiceConnection connection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className,
                                       IBinder service) {
            AudioService.AudioServiceBinder binder = (AudioService.AudioServiceBinder) service;
            audioService = binder.getService();
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            audioService = null;
        }
    };

    public static MainFragment newInstance() {
        return new MainFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        viewModel = new ViewModelProvider(this).get(MainViewModel.class);
        getContext().startForegroundService(new Intent(getContext(), AudioService.class));
    }

    @Override
    public void onStart() {
        super.onStart();
        Intent intent = new Intent(getContext(), AudioService.class);
        getContext().bindService(intent, connection, getContext().BIND_AUTO_CREATE);
    }

    @Override
    public void onStop() {
        super.onStop();
        getContext().unbindService(connection);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.fragment_main, container, false);
        for (final CardConfig cardConfig : cardConfigs) {
            final MaterialCardView materialCardView = (MaterialCardView) view.findViewById(cardConfig.getCardId());
            final Slider slider = (Slider) view.findViewById(cardConfig.getSliderId());
            materialCardView.setChecked(true);
            slider.setVisibility(View.INVISIBLE);
            materialCardView.setOnClickListener(v -> {
                if (materialCardView.isChecked()) {
                    materialCardView.setChecked(false);
                    slider.setVisibility(View.VISIBLE);
                    audioService.toggleChannel(cardConfig.getAudioChannel(), true);
                } else {
                    materialCardView.setChecked(true);
                    slider.setVisibility(View.INVISIBLE);
                    audioService.toggleChannel(cardConfig.getAudioChannel(), false);
                }
            });
            slider.addOnChangeListener((_s, v, _u) -> {
                audioService.setChannelVolume(cardConfig.getAudioChannel(), v);
            });
        }
        return view;
    }
}