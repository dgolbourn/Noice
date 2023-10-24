package uk.golbourn.noice.ui.main;

import androidx.lifecycle.ViewModelProvider;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.google.android.material.card.MaterialCardView;
import com.google.android.material.slider.Slider;

import uk.golbourn.noice.R;

public class MainFragment extends Fragment {

    private final CardModel[] cards = {
            new CardModel(R.id.card_1, R.id.slider_1),
            new CardModel(R.id.card_2, R.id.slider_2),
            new CardModel(R.id.card_3, R.id.slider_3),
            new CardModel(R.id.card_4, R.id.slider_4),
            new CardModel(R.id.card_5, R.id.slider_5),
            new CardModel(R.id.card_6, R.id.slider_6),
            new CardModel(R.id.card_7, R.id.slider_7),
            new CardModel(R.id.card_8, R.id.slider_8)};
    private MainViewModel viewModel;

    public static MainFragment newInstance() {
        return new MainFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        viewModel = new ViewModelProvider(this).get(MainViewModel.class);
        // TODO: Use the ViewModel
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.fragment_main, container, false);
        for (final CardModel card : cards) {
            final MaterialCardView materialCardView = (MaterialCardView) view.findViewById(card.getCardId());
            final Slider slider = (Slider) view.findViewById(card.getSliderId());
            materialCardView.setOnClickListener(v -> {
                if (materialCardView.isChecked()) {
                    materialCardView.setChecked(false);
                    slider.setVisibility(View.VISIBLE);
                } else {
                    materialCardView.setChecked(true);
                    slider.setVisibility(View.INVISIBLE);
                }
            });
        }
        return view;
    }
}