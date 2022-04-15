package com.ndk.audiotestapp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.os.Bundle;

import java.util.Timer;
import java.util.TimerTask;

public class HeadsetPlugReceiver extends BroadcastReceiver {

    MainActivity activity;
    /* Every time when USB Audio device plug or unplug, this Receiver will receive 3 intents. */
    int count = 0;

    public HeadsetPlugReceiver(MainActivity a)
    {
        activity = a;
    }

    @Override
    public void onReceive(Context context, Intent intent)
    {
        count++;
//        Bundle extras = intent.getExtras();
//        System.out.println("count: " + count);
//        for (String key: extras.keySet())
//        {
//            System.out.println("Key=" + key + ", content=" + extras.getString(key));
//        }

        if(count == 3)
        {
            redirectionUSBAudioDevice();
            count = 0;
        }
    }

    public void redirectionUSBAudioDevice()
    {
        activity.restartUSBAudioDevice();
    }
}