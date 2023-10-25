package uk.golbourn.noice.ui.main;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;

import uk.golbourn.noice.MainActivity;
import uk.golbourn.noice.R;

public class AudioService extends Service {

    private final IBinder binder = new AudioServiceBinder();

    public AudioService() {
    }

    public void toggleChannel(int i, boolean isPlaying) {
        System.out.println("Channel " + i + " " + (isPlaying ? "is Playing" : "is Paused"));
    }

    public void setChannelVolume(int i, float volume) {
        System.out.println("Channel " + i + " volume is " + volume);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel =
                    new NotificationChannel(getString(R.string.notification_channel),
                            getString(R.string.notification_channel),
                            NotificationManager.IMPORTANCE_DEFAULT);
            channel.setDescription(getString(R.string.notification_channel));
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }

        Intent alertIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, alertIntent, PendingIntent.FLAG_IMMUTABLE);

        Notification notification =
                new NotificationCompat.Builder(this, getString(R.string.notification_channel))
                        .setSmallIcon(R.drawable.notification)
                        .setContentTitle(getString(R.string.notification_title))
                        .setContentText(getString(R.string.notification_text))
                        .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                        .setContentIntent(pendingIntent)
                        .setAutoCancel(true)
                        .build();
        ServiceCompat.startForeground(
                this,
                42,
                notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK
        );
        return START_STICKY;
    }

    public class AudioServiceBinder extends Binder {
        AudioService getService() {
            return AudioService.this;
        }
    }
}