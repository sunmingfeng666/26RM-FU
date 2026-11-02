// Read USB identity via Windows hub GET_DESCRIPTOR requests, without claiming the
// debug interface. USB_NODE_CONNECTION_INFORMATION_EX is packed (status at 31).
// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/usbioctl/ns-usbioctl-_usb_descriptor_request
using System;
using System.Text;
using System.Runtime.InteropServices;
namespace ClionFlash {
    public static class Pnp {
        [StructLayout(LayoutKind.Sequential)]
        struct PropertyKey { public Guid format; public uint id; }
        [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
        static extern uint CM_Locate_DevNodeW(out uint node, string id, uint flags);
        [DllImport("cfgmgr32.dll")]
        static extern uint CM_Get_Parent(out uint parent, uint node, uint flags);
        [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
        static extern uint CM_Get_Device_IDW(uint node, StringBuilder id, uint length, uint flags);
        [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
        static extern uint CM_Get_DevNode_PropertyW(uint node, ref PropertyKey key, out uint type, byte[] buffer, ref uint length, uint flags);
        public static string Description(string id) {
            uint node, type, length=2048;
            if (String.IsNullOrEmpty(id) || CM_Locate_DevNodeW(out node, id, 0) != 0) return "";
            var key = new PropertyKey { format=new Guid("540B947E-8B40-45BC-A8A2-6A0B894CBDA2"), id=4 };
            var buffer = new byte[length];
            if (CM_Get_DevNode_PropertyW(node, ref key, out type, buffer, ref length, 0) != 0 || type != 18) return "";
            return Encoding.Unicode.GetString(buffer, 0, (int)length).TrimEnd('\0');
        }
        public static string Parent(string id) {
            uint node, parent;
            if (CM_Locate_DevNodeW(out node, id, 0) != 0 || CM_Get_Parent(out parent, node, 0) != 0) return "";
            var buffer = new StringBuilder(512);
            return CM_Get_Device_IDW(parent, buffer, 512, 0) == 0 ? buffer.ToString() : "";
        }

        [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
        static extern uint CM_Get_Device_Interface_List_SizeW(out uint size, ref Guid cls, string id, uint flags);
        [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
        static extern uint CM_Get_Device_Interface_ListW(ref Guid cls, string id, char[] buffer, uint size, uint flags);
        [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern Microsoft.Win32.SafeHandles.SafeFileHandle CreateFileW(string name, uint access, uint share, IntPtr security, uint disposition, uint flags, IntPtr template);
        [DllImport("kernel32.dll", SetLastError=true)]
        static extern bool DeviceIoControl(Microsoft.Win32.SafeHandles.SafeFileHandle handle, uint code, byte[] input, uint inputLength, byte[] output, uint outputLength, out uint returned, IntPtr overlapped);
        static uint Address(string id) {
            uint node, type, length=4;
            if (CM_Locate_DevNodeW(out node, id, 0) != 0) throw new Exception("USB device disappeared: " + id);
            var key = new PropertyKey { format=new Guid("A45C254E-DF1C-4EFD-8020-67D146A850E0"), id=30 };
            var data = new byte[4];
            if (CM_Get_DevNode_PropertyW(node, ref key, out type, data, ref length, 0) != 0 || type != 7)
                throw new Exception("Cannot read USB port address: " + id);
            return BitConverter.ToUInt32(data,0);
        }
        // Physical port chain from the root hub. This is NOT a serial number.
        public static string PortPath(string physicalId) {
            var ports=new System.Collections.Generic.List<string>();
            string id=physicalId;
            while (id.StartsWith("USB\\VID_",StringComparison.OrdinalIgnoreCase)) {
                uint port=Address(id);
                if (port==0 || port>255 || ports.Count>=7) throw new Exception("Invalid USB port chain");
                ports.Add(port.ToString(System.Globalization.CultureInfo.InvariantCulture));
                id=Parent(id);
            }
            if (!id.StartsWith("USB\\ROOT_HUB",StringComparison.OrdinalIgnoreCase) || ports.Count==0)
                throw new Exception("Cannot resolve the physical root USB hub");
            ports.Reverse();
            return String.Join(".",ports.ToArray());
        }
        static byte[] Descriptor(Microsoft.Win32.SafeHandles.SafeFileHandle hub, uint port, byte index, ushort language) {
            var data=new byte[12+256];
            Array.Copy(BitConverter.GetBytes(port),data,4);
            data[4]=0x80; data[5]=6;
            Array.Copy(BitConverter.GetBytes((ushort)(0x0300|index)),0,data,6,2);
            Array.Copy(BitConverter.GetBytes(language),0,data,8,2);
            Array.Copy(BitConverter.GetBytes((ushort)256),0,data,10,2);
            uint returned;
            if (!DeviceIoControl(hub,0x220410,data,(uint)data.Length,data,(uint)data.Length,out returned,IntPtr.Zero))
                throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error(),"Read USB string descriptor failed");
            if (returned<14 || data[13]!=3 || data[12]<2 || data[12]+12>returned || data[12]%2!=0)
                throw new Exception("Invalid USB string descriptor");
            var result=new byte[data[12]-2];
            Array.Copy(data,14,result,0,result.Length);
            return result;
        }
        // Read only standard descriptors through the parent hub. No interface claim,
        // CMSIS-DAP command, target connection, reset, or driver changes.
        public static string Serial(string physicalId) {
            uint port=Address(physicalId), size;
            string parent=Parent(physicalId);
            var cls=new Guid("F18A0E88-C30C-11D0-8815-00A0C906BED8");
            if (CM_Get_Device_Interface_List_SizeW(out size,ref cls,parent,0)!=0 || size<2)
                throw new Exception("Cannot find parent USB hub: " + physicalId);
            var names=new char[size];
            if (CM_Get_Device_Interface_ListW(ref cls,parent,names,size,0)!=0)
                throw new Exception("Cannot read parent USB hub path: " + physicalId);
            string path=new string(names).Split('\0')[0];
            using (var hub=CreateFileW(path,0,3,IntPtr.Zero,3,0,IntPtr.Zero)) {
                if (hub.IsInvalid) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error(),"Open parent USB hub failed");
                var data=new byte[4096];
                Array.Copy(BitConverter.GetBytes(port),data,4);
                uint returned;
                if (!DeviceIoControl(hub,0x220448,data,(uint)data.Length,data,(uint)data.Length,out returned,IntPtr.Zero))
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error(),"Read USB device descriptor failed");
                if (returned<35 || data[4]!=18 || data[5]!=1 || BitConverter.ToUInt32(data,31)!=1)
                    throw new Exception("USB port has no connected device: " + physicalId);
                ushort vid=BitConverter.ToUInt16(data,12), pid=BitConverter.ToUInt16(data,14);
                if (physicalId.IndexOf(String.Format("VID_{0:X4}&PID_{1:X4}",vid,pid),StringComparison.OrdinalIgnoreCase)<0)
                    throw new Exception("USB device changed while reading descriptors");
                byte index=data[20];
                if (index==0) return "";
                var languages=Descriptor(hub,port,0,0);
                if (languages.Length<2) throw new Exception("USB descriptor has no language ID");
                string serial=Encoding.Unicode.GetString(Descriptor(hub,port,index,BitConverter.ToUInt16(languages,0)));
                string instanceSerial=physicalId.Substring(physicalId.LastIndexOf('\\')+1);
                if (instanceSerial.IndexOf('&')<0 && !String.Equals(serial,instanceSerial,StringComparison.OrdinalIgnoreCase))
                    throw new Exception("USB device identity changed while reading its serial descriptor");
                return serial;
            }
        }
    }
}
