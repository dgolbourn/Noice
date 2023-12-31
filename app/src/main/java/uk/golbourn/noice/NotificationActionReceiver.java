package uk.golbourn.noice;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class NotificationActionReceiver extends BroadcastReceiver {
    private static native void start();

    private static native void stop();

    @Override
    public void onReceive(Context context, Intent intent) {
        switch(intent.getAction()){
            case "uk.golbourn.Intent.Action.Pause": stop(); break;
            case "uk.golbourn.Intent.Action.Resume": start(); break;
            default: break;
        }
    }
}
