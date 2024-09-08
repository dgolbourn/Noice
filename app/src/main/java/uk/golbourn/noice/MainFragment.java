package uk.golbourn.noice;

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

import com.google.android.material.card.MaterialCardView;
import com.google.android.material.slider.Slider;

public class MainFragment extends Fragment {

    private AudioService audioService;
    private final ServiceConnection connection = new ServiceConnection() {

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
        Intent startServiceIntent = new Intent(getContext(), AudioService.class);
        startServiceIntent.setAction("uk.golbourn.Intent.Action.Start");
        getContext().startForegroundService(startServiceIntent);
    }

    @Override
    public void onStart() {
        super.onStart();
        Intent intent = new Intent(getContext(), AudioService.class);
        getContext().bindService(intent, connection, getContext().BIND_AUTO_CREATE);
    }

    @Override
    public void onStop() {
        getContext().unbindService(connection);
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Intent stopSeviceIntent = new Intent(getContext(), AudioService.class);
        stopSeviceIntent.setAction("uk.golbourn.Intent.Action.Stop");
        getContext().startService(stopSeviceIntent);
        super.onDestroy();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.fragment_main, container, false);
        for (final CardConfig cardConfig : CardConfig.cardConfigs) {
            final MaterialCardView materialCardView = view.findViewById(cardConfig.cardId());
            final Slider slider = view.findViewById(cardConfig.sliderId());
            materialCardView.setChecked(true);
            slider.setVisibility(View.INVISIBLE);
            materialCardView.setOnClickListener(v -> {
                if (materialCardView.isChecked()) {
                    materialCardView.setChecked(false);
                    slider.setVisibility(View.VISIBLE);
                    audioService.toggleChannel(cardConfig.audioChannel(), true);
                } else {
                    materialCardView.setChecked(true);
                    slider.setVisibility(View.INVISIBLE);
                    audioService.toggleChannel(cardConfig.audioChannel(), false);
                }
            });
            slider.addOnChangeListener((_s, v, _u) -> {
                audioService.setChannelVolume(cardConfig.audioChannel(), v);
            });
        }
        return view;
    }
}