package com.ndk.audiotestapp;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/* 听筒和免提切换 */
public class HandsetHandFreeSwitch
{
    static final String HANDSET_IO_DIR_FILE_PATHNAME = "/sys/class/gpio/gpio36/direction";
    static final String HANDSET_IO_VALUE_FILE_PATHNAME = "/sys/class/gpio/gpio36/value";
    static final String HANDFREE_IO_DIR_FILE_PATHNAME = "/sys/class/gpio/gpio40/direction";
    static final String HANDFREE_IO_VALUE_FILE_PATHNAME = "/sys/class/gpio/gpio40/value";

    /* 使用手柄听筒通路 */
    static void useHandset()
    {
        HandFreeOff();
        HandsetOn();
    }

    /* 使用免提MIC通路 */
    static void useHandFree()
    {
        HandsetOff();
        HandFreeOn();
    }

    static void HandsetOn()
    {
        setIODirOut(HANDSET_IO_DIR_FILE_PATHNAME);
        setIOValue(HANDSET_IO_VALUE_FILE_PATHNAME, "1");
    }

    static void HandsetOff()
    {
        setIODirOut(HANDSET_IO_DIR_FILE_PATHNAME);
        setIOValue(HANDSET_IO_VALUE_FILE_PATHNAME, "0");
    }

    static void HandFreeOn()
    {
        setIODirOut(HANDFREE_IO_DIR_FILE_PATHNAME);
        setIOValue(HANDFREE_IO_VALUE_FILE_PATHNAME, "1");
    }

    static void HandFreeOff()
    {
        setIODirOut(HANDFREE_IO_DIR_FILE_PATHNAME);
        setIOValue(HANDFREE_IO_VALUE_FILE_PATHNAME, "0");
    }

    static boolean setIODirOut(String pathname)
    {
        boolean isout = false;
        File dir = new File(pathname);
        if(!dir.exists())
        {
            System.out.println(pathname + "doesn't exist.");
            return false;
        }

        try(FileInputStream fin = new FileInputStream(dir)) {
            byte[] bread = new byte[3];
            int ret = fin.read(bread);
            String curDir = new String(bread);
            if(!curDir.equals("out"))
            {
                String out = "out";
                byte[] bout = out.getBytes();

                try(FileOutputStream fos = new FileOutputStream(dir)) {
                    fos.write(bout);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            else
            {
                System.out.println("already out");
                return true;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        try(FileInputStream fin = new FileInputStream(dir))
        {
            byte[] bread = new byte[3];
            int ret = fin.read(bread);
            String curDir = new String(bread);
            if (!curDir.equals("out"))
            {
                System.out.println("set out failed.");
                isout = false;
            }
            else
            {
                System.out.println("set out successfully.");
                isout = true;
            }
        }catch (IOException e) {
            e.printStackTrace();
        }
        return isout;
    }

    static void setIOValue(String pathname, String value)
    {
        File val = new File(pathname);
        if(!val.exists())
        {
            System.out.println(pathname + "doesn't exist.");
        }

        try(FileInputStream fin = new FileInputStream(val)) {
            byte[] bread = new byte[3];
            int ret = fin.read(bread);
            String curVal = new String(bread);
            if(!curVal.equals(value))
            {
                byte[] bout = value.getBytes();

                try(FileOutputStream fos = new FileOutputStream(val)) {
                    fos.write(bout);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
