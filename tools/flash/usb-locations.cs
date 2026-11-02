// Enumerate topology with the SAME libusb DLL shipped with the selected OpenOCD.
// No libusb_open, interface claim, control transfer, target command or USB reset.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
namespace ClionFlash {
    public sealed class UsbLocation {
        public ushort Vendor, Product;
        public string PortPath, Location;
    }
    public static class UsbLocations {
        [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern IntPtr LoadLibraryW(string path);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern int libusb_init(out IntPtr context);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern void libusb_exit(IntPtr context);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern IntPtr libusb_get_device_list(IntPtr context, out IntPtr list);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern void libusb_free_device_list(IntPtr list, int unref);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern int libusb_get_device_descriptor(IntPtr device, [Out] byte[] descriptor);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern byte libusb_get_bus_number(IntPtr device);
        [DllImport("libusb-1.0.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern int libusb_get_port_numbers(IntPtr device, [Out] byte[] ports, int length);
        static string loadedPath;
        public static UsbLocation[] Read(string dllPath) {
            dllPath=System.IO.Path.GetFullPath(dllPath);
            if (loadedPath!=null && !String.Equals(loadedPath,dllPath,StringComparison.OrdinalIgnoreCase))
                throw new Exception("Use a fresh PowerShell process when changing the OpenOCD libusb DLL");
            if (loadedPath==null) {
                if (LoadLibraryW(dllPath)==IntPtr.Zero)
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error(),"Cannot load OpenOCD libusb");
                loadedPath=dllPath;
            }
            IntPtr context, list=IntPtr.Zero;
            int result=libusb_init(out context);
            if (result!=0) throw new Exception("libusb_init failed: " + result);
            try {
                long count=libusb_get_device_list(context,out list).ToInt64();
                if (count<0 || count>65535) throw new Exception("USB enumeration failed: " + count);
                var found=new List<UsbLocation>();
                for (int i=0;i<count;i++) {
                    IntPtr device=Marshal.ReadIntPtr(list,i*IntPtr.Size);
                    byte[] descriptor=new byte[18], ports=new byte[7];
                    if (libusb_get_device_descriptor(device,descriptor)!=0) continue;
                    ushort vendor=BitConverter.ToUInt16(descriptor,8), product=BitConverter.ToUInt16(descriptor,10);
                    if (vendor!=0x0483) continue;
                    int depth=libusb_get_port_numbers(device,ports,ports.Length);
                    if (depth<=0) continue;
                    var path=new string[depth];
                    for (int j=0;j<depth;j++) path[j]=ports[j].ToString(System.Globalization.CultureInfo.InvariantCulture);
                    string portPath=String.Join(".",path);
                    found.Add(new UsbLocation {Vendor=vendor,Product=product,PortPath=portPath,
                        Location=libusb_get_bus_number(device).ToString(System.Globalization.CultureInfo.InvariantCulture)+"-"+portPath});
                }
                return found.ToArray();
            } finally {
                if (list!=IntPtr.Zero) libusb_free_device_list(list,1);
                libusb_exit(context);
            }
        }
    }
}
