package uk.golbourn.noice;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ServiceInfo;
import android.content.res.AssetManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;

import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;

import java.util.Arrays;

public class AudioService extends Service {

    static {
        System.loadLibrary("native-lib");
    }
    private static native void lazyInitialise(String[] files, AssetManager assetManager);

    private static native void quickInitialise(String[] files, AssetManager assetManager);

    private static native void initialise();

    private static native void destroy();

    private static native void start();

    private static native void stop();

    private static native void setNativeChannelVolume(int i, float Volume);

    private static native void setNativeChannelPlaying(int i, boolean isPlaying);

    private final IBinder binder = new AudioServiceBinder();

    private final BroadcastReceiver receiver = new AudioServiceBroadcastReceiver();

    private Thread lazyInitialiserThread = null;

    public AudioService() {
    }


    public void toggleChannel(int i, boolean isPlaying) {
        setNativeChannelPlaying(i, isPlaying);
        start();
    }

    public void setChannelVolume(int i, float volume) {
        setNativeChannelVolume(i, volume);
        start();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        initialise();
        start();

        IntentFilter filter = new IntentFilter();
        filter.addAction("uk.golbourn.Intent.Action.Pause");
        filter.addAction("uk.golbourn.Intent.Action.Resume");
        registerReceiver(receiver, filter, Context.RECEIVER_EXPORTED);

        lazyInitialiserThread = new Thread(() -> lazyInitialise(Arrays.stream(CardConfig.cardConfigs).map(CardConfig::getFileName).toArray(String[]::new), getAssets()));
        lazyInitialiserThread.start();
        quickInitialise(Arrays.stream(CardConfig.cardConfigs).map(CardConfig::getQuickFileName).toArray(String[]::new), getAssets());
    }

    @Override
    public void onDestroy() {
        try {
            if (null != lazyInitialiserThread) {
                lazyInitialiserThread.join();
                lazyInitialiserThread = null;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
        stop();
        destroy();
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        switch(intent.getAction()) {
            default:
            case "uk.golbourn.Intent.Action.Start":
                NotificationChannel channel = new NotificationChannel("Noice", "Noice", NotificationManager.IMPORTANCE_DEFAULT);
                NotificationManager notificationManager = getSystemService(NotificationManager.class);
                notificationManager.createNotificationChannel(channel);

                Intent contentIntent = new Intent(getApplicationContext(), MainActivity.class);
                contentIntent.setAction(Intent.ACTION_MAIN);
                contentIntent.addCategory(Intent.CATEGORY_LAUNCHER);
                contentIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                PendingIntent pendingContentIntent = PendingIntent.getActivity(getApplicationContext(), 43, contentIntent, PendingIntent.FLAG_IMMUTABLE);

                Intent pauseIntent = new Intent("uk.golbourn.Intent.Action.Pause");
                PendingIntent pendingPauseIntent = PendingIntent.getBroadcast(getApplicationContext(), 44, pauseIntent, PendingIntent.FLAG_IMMUTABLE);

                Intent resumeIntent = new Intent("uk.golbourn.Intent.Action.Resume");
                PendingIntent pendingResumeIntent = PendingIntent.getBroadcast(getApplicationContext(), 45, resumeIntent, PendingIntent.FLAG_IMMUTABLE);

                Notification notification = new NotificationCompat.Builder(this, "Noice")
                        .setSmallIcon(R.drawable.notification)
                        .setContentIntent(pendingContentIntent)
                        .addAction(R.drawable.pause, "Pause", pendingPauseIntent)
                        .addAction(R.drawable.play, "Resume", pendingResumeIntent)
                        .setSilent(true)
                        .setOngoing(true)
                        .build();
                ServiceCompat.startForeground(this, 42, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
            return START_STICKY;
            case "uk.golbourn.Intent.Action.Stop":
                ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE);
                stopSelf();
                return START_NOT_STICKY;
        }
    }

    private class AudioServiceBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            switch(intent.getAction()){
                case "uk.golbourn.Intent.Action.Pause": stop(); break;
                case "uk.golbourn.Intent.Action.Resume": start(); break;
                default: break;
            }
        }
    }

    public class AudioServiceBinder extends Binder {
        AudioService getService() {
            return AudioService.this;
        }
    }
}