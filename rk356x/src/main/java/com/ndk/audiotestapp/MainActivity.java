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
import android.content.pm.PackageManager;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.View;

import java.io.DataInputStream;
import java.io.DataOutputStream;
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
//            Manifest.permission.WRITE_EXTERNAL_STORAGE,
//            Manifest.permission.READ_EXTERNAL_STORAGE,
//            Manifest.permission.READ_PHONE_STATE,
            Manifest.permission.RECORD_AUDIO
    };

    public MyAudioRecord myAudioRecord0 = new MyAudioRecord(ROUTE_OUR_SIDE_TO_OPPOSITE_SIZE,
                                                            SOUND_DEV_IN_ON_BOARD_MIC);

    public MyAudioRecord myAudioRecord1 = new MyAudioRecord(ROUTE_OPPOSITE_SIZE_TO_OUR_SIDE,
                                                            SOUND_DEV_IN_USB_MIC);

    public MyAudioTrack myAudioTrack0 = new MyAudioTrack(ROUTE_OUR_SIDE_TO_OPPOSITE_SIZE,
                                                            SOUND_DEV_OUT_USB_SPK);

    public MyAudioTrack myAudioTrack1 = new MyAudioTrack(ROUTE_OPPOSITE_SIZE_TO_OUR_SIDE,
                                                            SOUND_DEV_OUT_ON_BOARD_SPK);

    public CaptureThread captureThread0;
    public CaptureThread captureThread1;

    public PlayThread playThread0;
    public PlayThread playThread1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
//
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)!=
                PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, REQUEST_PERMISSIONS, REQUEST_CODE_ASK_PERMISSIONS);
        }
//        MyAudioRecord.AudioDevFilePermissionGet();
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
                captureThread0 = new CaptureThread(ROUTE_OUR_SIDE_TO_OPPOSITE_SIZE, myAudioRecord0);
                captureThread0.start();
                break;
            case R.id.buttonRoute1Capture:
                if(myAudioRecord1.Capturing)
                    return;
                myAudioRecord1.startRecording(myAudioRecord1.sndDevice);
                captureThread1 = new CaptureThread(1, myAudioRecord1);
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
                myAudioTrack0.startPlaying(myAudioTrack0.sndDevice);
                playThread0 = new PlayThread(0, myAudioTrack0);
                playThread0.start();
                break;
            case R.id.buttonRoute1Play:
                if(myAudioTrack1.Playing)
                    return;
                myAudioTrack1.startPlaying(myAudioTrack1.sndDevice);
                playThread1 = new PlayThread(1, myAudioTrack1);
                playThread1.start();
                break;
            case R.id.buttonExchangeP:
                myAudioTrack1.changeTinyALSADeviceP(SOUND_DEV_OUT_HEADPHONE_SPK);
                break;
            default:
                break;
        }
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

    private final MyAudioRecord audioRecord;
    public CaptureThread(int route_set, MyAudioRecord AudioRecord)
    {
        route = route_set;
        audioRecord = AudioRecord;
    }
    @Override
    public void run() {
            while(this.audioRecord.Capturing)
            {
                byte[] dataRead = this.audioRecord.read();
                AudioDataBuffer.setData(dataRead, route);
            }
        }
}

class PlayThread extends Thread {

    private final int route;
    private final MyAudioTrack audioTrack;
    private final byte [] foo;
    public PlayThread(int route_set, MyAudioTrack AudioTrack)
    {
        this.route = route_set;
        this.audioTrack = AudioTrack;
        foo = new byte[]{1, 2, 3, 4, 5, 6, 7, 8};
    }
    @Override
    public void run() {
        while(this.audioTrack.Playing) {
            if(!AudioDataBuffer.isDataFetched(this.route)) {
                 byte[] data = AudioDataBuffer.getData(this.route);
                 this.audioTrack.write(data);
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
