package uk.golbourn.noice;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Binder;
import android.os.IBinder;

import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;

public class AudioService extends Service {
    private static native void initialise();

    private static native void destroy();

    private static native void start();

    private static native void stop();

    private static native void setNativeChannelVolume(int i, float Volume);

    private static native void setNativeChannelPlaying(int i, boolean isPlaying);

    private final IBinder binder = new AudioServiceBinder();

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
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stop();
        destroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        NotificationChannel channel = new NotificationChannel("Noice", "Noice", NotificationManager.IMPORTANCE_DEFAULT);
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);

        Intent contentIntent = new Intent(getApplicationContext(), AudioService.class);
        PendingIntent pendingContentIntent = PendingIntent.getActivity(getApplicationContext(), 43, contentIntent, PendingIntent.FLAG_IMMUTABLE);

        Intent stopIntent = new Intent(getApplicationContext(), NotificationActionReceiver.class);
        stopIntent.setAction("Stop");
        PendingIntent pendingStopIntent = PendingIntent.getBroadcast(getApplicationContext(), 44, stopIntent, PendingIntent.FLAG_IMMUTABLE);

        Intent continueIntent = new Intent(getApplicationContext(), NotificationActionReceiver.class);
        continueIntent.setAction("Continue");
        PendingIntent pendingContinueIntent = PendingIntent.getBroadcast(getApplicationContext(), 45, continueIntent, PendingIntent.FLAG_IMMUTABLE);

        Notification notification = new NotificationCompat.Builder(this, "Noice")
                .setSmallIcon(R.drawable.notification)
                .setContentIntent(pendingContentIntent)
                .addAction(R.drawable.pause, "Pause", pendingStopIntent)
                .addAction(R.drawable.play, "Resume", pendingContinueIntent)
                .setOngoing(true)
                .build();
        ServiceCompat.startForeground(this, 42, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
        return START_STICKY;
    }

    public class AudioServiceBinder extends Binder {
        AudioService getService() {
            return AudioService.this;
        }
    }
}