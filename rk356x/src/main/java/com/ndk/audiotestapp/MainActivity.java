package com.ndk.audiotestapp;

import static com.ndk.audiotestapp.MyAudioRecord.*;
import static com.ndk.audiotestapp.MyAudioTrack.*;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.View;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

//import com.ndk.audiotestapp.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'audiotestapp' library on application startup.
    static {
        System.loadLibrary("audiotestapp");
    }

    private final int REQUEST_CODE_ASK_PERMISSIONS = 128;
    private static final String[] REQUEST_PERMISSIONS = new String[]{
//            Manifest.permission.MANAGE_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
//            Manifest.permission.READ_PHONE_STATE,
            Manifest.permission.RECORD_AUDIO,
    };

    public MyAudioRecord myAudioRecord0 = new MyAudioRecord(0);

    public MyAudioRecord myAudioRecord1 = new MyAudioRecord(1);

    public MyAudioTrack myAudioTrack0 = new MyAudioTrack(0);

    public MyAudioTrack myAudioTrack1 = new MyAudioTrack(1);

    public CaptureThread captureThread0;
    public CaptureThread captureThread1;

    public PlayThread playThread0;
    public PlayThread playThread1;

    HeadsetPlugReceiver receiver;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
//
        for(String permission : REQUEST_PERMISSIONS) {
            if (ContextCompat.checkSelfPermission(this, permission) !=
                    PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, REQUEST_PERMISSIONS, REQUEST_CODE_ASK_PERMISSIONS);
            }
        }

        configureReceiver();
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(receiver);
    }

    private void configureReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction("android.intent.action.HEADSET_PLUG");
        receiver = new HeadsetPlugReceiver(this);
        registerReceiver(receiver, filter);
    }
    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults)
    {
        if(requestCode != REQUEST_CODE_ASK_PERMISSIONS) {
            finish();
            return;
        }

        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                finish();
                return;
            }
        }
    }
    public void restartUSBAudioDevice()
    {
        int DeviceID;
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if(myAudioRecord1.Capturing)
        {
            myAudioRecord1.stopRecording();
            DeviceID = myAudioRecord1.getAudioDeviceID(AudioDeviceInfo.TYPE_USB_HEADSET, audioManager);
            myAudioRecord1.startRecording(DeviceID);
            captureThread1 = new CaptureThread(1, myAudioRecord1, getExternalFilesDir(null));
            captureThread1.start();
        }

        if(myAudioTrack0.Playing)
        {
            myAudioTrack0.stopPlaying();
            DeviceID = myAudioTrack0.getAudioDeviceID(AudioDeviceInfo.TYPE_USB_HEADSET, audioManager);
            myAudioTrack0.startPlaying(DeviceID);
            playThread0 = new PlayThread(0, myAudioTrack0, getExternalFilesDir(null));
            playThread0.start();
        }
    }

    @SuppressLint("NonConstantResourceId")
    public void audioTestRecord(View view)
    {
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        int recordDeviceID;
        switch(view.getId())
        {
            case R.id.buttonRoute0Capture:
                if(myAudioRecord0.Capturing)
                    return;
                myAudioRecord0.startRecording(myAudioRecord0.sndDevice);
                captureThread0 = new CaptureThread(0, myAudioRecord0, getExternalFilesDir(null));
                captureThread0.start();
                break;
            case R.id.buttonRoute1Capture:
                if(myAudioRecord1.Capturing)
                    return;
                recordDeviceID = myAudioRecord1.getAudioDeviceID(AudioDeviceInfo.TYPE_USB_HEADSET, audioManager);
                myAudioRecord1.startRecording(recordDeviceID);
                captureThread1 = new CaptureThread(1, myAudioRecord1, getExternalFilesDir(null));
                captureThread1.start();
                break;
            case R.id.buttonExchangeC:
                myAudioRecord0.changeTinyALSADeviceC(SOUND_DEV_IN_HEADPHONE_MIC);
            default:
                break;
        }
    }

    @SuppressLint("NonConstantResourceId")
    public void audioTestPlay(View view)
    {
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        int playDeviceID;
        switch(view.getId())
        {
            case R.id.buttonRoute0Play:
                if(myAudioTrack0.Playing)
                    return;
                playDeviceID = myAudioTrack0.getAudioDeviceID(AudioDeviceInfo.TYPE_USB_HEADSET, audioManager);
                myAudioTrack0.startPlaying(playDeviceID);
                playThread0 = new PlayThread(0, myAudioTrack0, getExternalFilesDir(null));
                playThread0.start();
                break;
            case R.id.buttonRoute1Play:
                if(myAudioTrack1.Playing)
                    return;
                myAudioTrack1.startPlaying(myAudioTrack1.sndDevices);
                playThread1 = new PlayThread(1, myAudioTrack1, getExternalFilesDir(null));
                playThread1.start();
                break;
            case R.id.buttonExchangeP:
                myAudioTrack1.changeTinyALSADeviceP(SOUND_DEV_OUT_ON_BOARD_SPK);
                break;
            default:
                break;
//        }
//        for(AudioDeviceInfo adi : audioDeviceInfo)
//        {
//            if(adi.getType() == outputDeviceType) {
//                playDeviceID = adi.getId();
//                System.out.println("Device ID = ");
//                System.out.println(playDeviceID);
//                break;
//            }
            //System.out.println(adi.getType());
        }

//        AAudioCreateOutputStream(playDeviceID);
    }

    @SuppressLint("NonConstantResourceId")
    public void audioTestStopRecord(View view)
    {
        switch(view.getId())
        {
            case R.id.buttonRoute0CaptureStop:
                myAudioRecord0.stopRecording();
                break;
            case R.id.buttonRoute1CaptureStop:
                myAudioRecord1.stopRecording();
                break;
            default:
                break;
        }
    }

    @SuppressLint("NonConstantResourceId")
    public void audioTestStopPlay(View view)
    {
        switch(view.getId())
        {
            case R.id.buttonRoute0PlayStop:
                myAudioTrack0.stopPlaying();
                break;
            case R.id.buttonRoute1PlayStop:
                myAudioTrack1.stopPlaying();
                break;
            default:
                break;
        }
    }

    public static void saveBytesToFile(File EFD, String filename, byte[] data)
    {
        File file = new File(EFD, filename);

        if(!file.exists())
        {
            try {
                file.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        try(FileOutputStream fos = new FileOutputStream(file, true)) {
            fos.write(data);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static int readBytesFromFile(File EFD, String filename, byte[] data, int off)
    {
        File file = new File(EFD, filename);
        int ret = -1;

        try(FileInputStream fin = new FileInputStream(file)) {
            fin.skip(off);
            ret = fin.read(data);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return ret;
    }

    public static boolean execCommand(String command) {
        Process process = null;
        DataOutputStream os = null;
        DataInputStream is = null;
        try {
            process = Runtime.getRuntime().exec("su");
            os = new DataOutputStream(process.getOutputStream());
            os.writeBytes(command + "\n");
            os.writeBytes("exit\n");
            os.flush();
            int ret = process.waitFor();
            System.out.println("waitFor():");
            System.out.println(ret);
            is = new DataInputStream(process.getInputStream());
            byte[] buffer = new byte[is.available()];
            System.out.println("大小");
            System.out.println(buffer.length);
            is.read(buffer);
            String out = new String(buffer);
            System.out.println("返回:");
            System.out.println(out);

            /*File snd_dev = new File("/dev/snd/pcmC1D0c");
            if(snd_dev.canRead() && snd_dev.canWrite())
                System.out.println("sound device "+ snd_dev.getAbsolutePath() + " have rw permission.");
            else
                System.out.println("sound device "+ snd_dev.getAbsolutePath() + " haven't rw permission.");*/
        } catch (Exception e) {
            e.printStackTrace();
            System.out.println("205:\n");
            System.out.println(e);
            return false;
        } /**/finally {
            try {
                if (os != null) {
                    os.close();
                }
                if (is != null) {
                    is.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
                System.out.println("217:\n");
                System.out.println(e);
            }
            process.destroy();
        }
        System.out.println("Permission Check Success.");
        return true;
    }
}

class CaptureThread extends Thread {

    private final int route;
    private File EFD;
    private final MyAudioRecord audioRecord;
    public CaptureThread(int route_set, MyAudioRecord AudioRecord, File EFD_set)
    {
        route = route_set;
        audioRecord = AudioRecord;
        EFD = EFD_set;
    }
    @Override
    public void run() {
        if(route == 0) {
            while(this.audioRecord.Capturing)
            {
                byte[] dataRead = this.audioRecord.read();
                AudioDataBuffer.setData(dataRead, route);
            }
        }
        else {
            while(this.audioRecord.Capturing)
            {
                byte[] dataRead = this.audioRecord.read();
                MainActivity.saveBytesToFile(EFD, "voice_2.pcm" ,dataRead);
            }
        }
    }
}

class PlayThread extends Thread {

    private final int route;
    private final MyAudioTrack audioTrack;
    private File EFD;
    private int off;
    public PlayThread(int route_set, MyAudioTrack AudioTrack, File EFD_set)
    {
        this.route = route_set;
        this.audioTrack = AudioTrack;
        EFD = EFD_set;
        off = 0;
    }
    @Override
    public void run() {
        while(this.audioTrack.Playing) {
            byte[] data = new byte[2048];
            int ret = MainActivity.readBytesFromFile(EFD, "noise.pcm",
                                            data, off);
            if(ret != -1) {
                this.audioTrack.write(data);
                off += data.length;
            }
            else {
                off = 0;
                break;
            }
        }
    }
}

class AudioDataBuffer{
     private static byte [][] dataRead = new byte[2][4096];
     private static int [] dataSize = new int [2];
     private static boolean [] dataFetched = new boolean[2];

     public static void setData(byte [] data, int route){
         dataSize[route] = data.length;
         System.arraycopy(data, 0, dataRead[route], 0, dataSize[route]);
         dataFetched[route] = false;
     }

     public static byte[] getData(int route) {
         byte [] data = new byte[dataSize[route]];
         System.arraycopy(dataRead[route], 0, data, 0, data.length);
         dataFetched[route] = true;
         return data;
     }

     public static boolean isDataFetched(int route){
         return dataFetched[route];
     }
}
