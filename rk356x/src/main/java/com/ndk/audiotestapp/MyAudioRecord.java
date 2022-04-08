package com.ndk.audiotestapp;

import android.media.AudioDeviceInfo;
import android.media.AudioManager;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;

public class MyAudioRecord {
    /* Route Defination */
    public static final int ROUTE_OUR_SIDE_TO_OPPOSITE_SIZE = 0;
    public static final int ROUTE_OPPOSITE_SIZE_TO_OUR_SIDE = 1;

    /* Sound Device Defination */
    public static final int SOUND_DEV_IN_ON_BOARD_MIC = 0;
    public static final int SOUND_DEV_IN_HEADPHONE_MIC = 1;
    public static final int SOUND_DEV_IN_USB_MIC = 2;
    public static final int SOUND_DEV_IN_EX_USB_MIC = 3;

    private int Route;          // 0: board mic -> usb headset;
                                // 1: usb mic -> board spk
    public int sndDevice;
    public boolean Capturing = false;
    public byte [] CapturedData = {0};
    // Used to load the 'audiotestapp' library on application startup.
    static {
        System.loadLibrary("audiotestapp");
    }

    // 使用底层 C 库(tinyalsa)录音
    public native void TinyALSAOpenDeviceC(int route, int device);
    // 使用 NDK AAudio 库录音
    public native void AAudioOpenDeviceC(int route, int deviceID);


    public native void TinyALSACloseDeviceC(int route);

    public native byte [] TinyALSARead(int route);

    public MyAudioRecord(int Route, int dev)
    {
        this.Route = Route;
        this.sndDevice = dev;
    }

    public static boolean RootCommand(String command) {
        /**/Process process = null;
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

    public static void AudioDevFilePermissionGet()
    {
        String snd_dev_permission = "chmod 666 /dev/snd/*\n" + "exit\n";
                /*
                "chmod 666 /dev/snd/controlC0\n" +
                "chmod 777 /dev/snd/controlC3\n" +
                "chmod 777 /dev/snd/pcmC0D0c\n" +
                "chmod 777 /dev/snd/pcmC0D0p\n" +
                "chmod 777 /dev/snd/pcmC3D0c\n" +
                "chmod 777 /dev/snd/pcmC3D0p\n" + "exit\n";*/

        boolean b = RootCommand(snd_dev_permission);
        System.out.println("获取音频设备文件权限:");
        System.out.println(b);
    }

    /** if use tinyALSA, parameter "Device" is a custom defined constant integer, see
     *  SOUND_DEV_IN_ON_BOARD_MIC, SOUND_DEV_IN_HEADPHONE_MIC and so on.
     *  if use AAudio, parameter "Device" is the device id obtained through
     *  AudioDeviceInfo.getDevices method.
     */
    public void startRecording (int Device)
    {
        if(this.Capturing == true)
            return;
        // required sample rate = 16kHz, required channels number = 1
        this.Capturing = true;
        this.TinyALSAOpenDeviceC(this.Route, Device);
    }

    public void stopRecording()
    {
        if(this.Capturing == false)
            return;

            this.Capturing = false;
            // TODO 等待 read 线程退出
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            TinyALSACloseDeviceC(this.Route);
    }
    public byte[] read ()
    {
        return this.CapturedData = this.TinyALSARead(this.Route);
    }

    public int getAudioDeviceID(int audioDeviceType, AudioManager audioManager)
    {
        int recordDeviceID = 0;
        AudioDeviceInfo audioDeviceInfo[] = audioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);

        for(AudioDeviceInfo adi : audioDeviceInfo)
        {
            if(adi.getType() == audioDeviceType) {
                recordDeviceID = adi.getId();
                System.out.println("Device ID = ");
                System.out.println(recordDeviceID);
                break;
            }
        }
        return recordDeviceID;
    }

    public void changeTinyALSADeviceC(int device)
    {
        if(this.Route != 0)
        {
            System.out.println("This route doesn't use lib tinyALSA to capture.\n");
            return;
        }
        this.stopRecording();

        this.sndDevice = device;
    }
}
