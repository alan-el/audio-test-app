package com.ndk.audiotestapp;

import android.media.AudioDeviceInfo;
import android.media.AudioManager;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class MyAudioTrack
{
    public static final int SOUND_DEV_OUT_HEADPHONE_SPK = 10;
    public static final int SOUND_DEV_OUT_ON_BOARD_SPK = 11;
    public static final int SOUND_DEV_OUT_USB_SPK = 12;

    private int Route;          // 0: board mic(tinyALSA) -> usb headset(AAudio) ;
                                // 1: usb mic(AAudio) -> board spk(tinyALSA)
    private byte [] TrackDataBuffer;
    private int sndDevices;
    private int writeIndex;
    public boolean Playing = false;
    // Used to load the 'audiotestapp' library on application startup.
    static {
        System.loadLibrary("audiotestapp");
    }

    // 使用底层 C 库(tinyalsa)播放
    public native void TinyALSAOpenDeviceP(int route, int device);
    // 使用 NDK AAudio 库播放
    public native void AAudioOpenDeviceP(int route, int deviceID);


    public native void TinyALSACloseDeviceP(int route);
    public native void AAudioCloseDeviceP();

    public native void TinyALSAWrite(int route, byte [] data);
    public native void AAudioWrite(int route, byte [] data, int size);

    public MyAudioTrack(int Route)
    {
        this.Route = Route;
        this.TrackDataBuffer = new byte[4096];
        this.writeIndex = 0;
    }
    /** if use tinyALSA, parameter "Device" is a custom defined constant integer, see
     *  SOUND_DEV_OUT_HEADPHONE_SPK, SOUND_DEV_OUT_ON_BOARD_SPK and so on.
     *  if use AAudio, parameter "Device" is the device id obtained through
     *  AudioDeviceInfo.getDevices method.
     */
    public void startPlaying (int Device)
    {
        if(this.Playing == true)
            return;

        if(this.Route == 1)
        {
            this.Playing = true;
            this.TinyALSAOpenDeviceP(this.Route, Device);
            sndDevices = Device;
        }
        else
        {
            this.Playing = true;
            this.AAudioOpenDeviceP(this.Route, Device);
        }
    }

    public void stopPlaying()
    {
        if(this.Playing == false)
            return;

        if(this.Route == 1)
        {
            this.Playing = false;
            // 等待 write 线程退出 TODO 延时时间确定
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            TinyALSACloseDeviceP(this.Route);
        }
        else
        {
            this.Playing = false;
            // 等待 write 线程退出 TODO 延时时间确定
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            AAudioCloseDeviceP();
        }
    }
    public int write (byte[] audioData)
    {
        if(this.Route == 1)
        {
            byte [] doubleChannelsData = expandSingleChannelToDouble(audioData);
            // 使用 TinyALSA 播放时，当写的数据长度不为 PERIOD_SIZE * PERIOD_COUNT * 2 * channels num 时，
            // 不能发送，要积累到要求值才能发送，PERIOD_SIZE 和 PERIOD_COUNT 可以在 ndk_aaudio.h 中设置，
            // 若修改，成员变量TrackDataBuffer 初始化时的长度也要修改，目前的值是 512 * 2 * 2 * 2 = 4096
            // 如果每次调用 write 时 sizeInBytes != 4096，可能会产生周期性噪声？
            int dataIndex = 0;
            while(dataIndex <= (doubleChannelsData.length - 1)) {
                this.TrackDataBuffer[this.writeIndex] = doubleChannelsData[dataIndex];

                if(this.writeIndex == this.TrackDataBuffer.length - 1) {
                    this.TinyALSAWrite(this.Route, this.TrackDataBuffer);
                    this.writeIndex = -1;
                }

                this.writeIndex++;
                dataIndex++;
            }
        }
        else
        {
            this.AAudioWrite(this.Route, audioData, 1024);
        }
        return 0;
    }

    public int getAudioDeviceID(int audioDeviceType, AudioManager audioManager)
    {
        int playDeviceID = 0;
        AudioDeviceInfo audioDeviceInfo[] = audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);

        for(AudioDeviceInfo adi : audioDeviceInfo)
        {
            if(adi.getType() == audioDeviceType) {
                playDeviceID = adi.getId();
                System.out.println("Device ID = ");
                System.out.println(playDeviceID);
                break;
            }
        }
        return playDeviceID;
    }

    // 单声道pcm数据(srcData)扩展成双声道pcm数据(dstData)
    private byte [] expandSingleChannelToDouble(byte [] srcData)
    {
        byte [] dstData = new byte[2 * srcData.length];

        int index = 0;
        for(int i = 0; i < srcData.length; i += 2 , index += 4)
        {
            dstData[index] = 0/*srcData[i]*/;
            dstData[index + 2] = srcData[i];

            dstData[index + 1] = 0/*srcData[i + 1]*/;
            dstData[index + 3] = srcData[i + 1];
        }

        return dstData;
    }

    public void changeTinyALSADeviceP(int device)
    {
        if(this.Route != 1)
        {
            System.out.println("This route doesn't use lib tinyALSA to track.\n");
            return;
        }
        this.stopPlaying();

        this.startPlaying(device);
    }
}
