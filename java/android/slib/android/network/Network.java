/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

package slib.android.network;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import android.os.Build;
import android.util.Log;

public class Network {

	public static NetworkDevice[] getAllDevices() {
		ArrayList<NetworkDevice> devices = new ArrayList<NetworkDevice>();
		try {
			Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces();
			for (; en.hasMoreElements();) {
				NetworkDevice device = new NetworkDevice();
				NetworkInterface iface = en.nextElement();
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
					device.index = iface.getIndex();
				} else {
					device.index = 0;
				}
				device.name = iface.getName();
				ArrayList<String> addr4 = new ArrayList<String>();
				ArrayList<String> addr6 = new ArrayList<String>();
				List<InterfaceAddress> list = iface.getInterfaceAddresses();
				for (int i = 0; i < list.size(); i++) {
					InterfaceAddress addr = list.get(i);
					InetAddress inetAddress = addr.getAddress();
					if (inetAddress != null) {
						if (!inetAddress.isLoopbackAddress()) {
							if (inetAddress instanceof Inet4Address) {
								int p = addr.getNetworkPrefixLength();
								String s = inetAddress.getHostAddress() + "/" + p;
								addr4.add(s);
							} else if (inetAddress instanceof Inet6Address) {
								int p = addr.getNetworkPrefixLength();
								String s = inetAddress.getHostAddress();
								int indexScope = s.indexOf('%');
								if (indexScope > 0) {
									s = s.substring(0, indexScope);
								}
								s += "/" + p;
								addr6.add(s);
							}
						}
					}
				}
				byte[] macAddr = iface.getHardwareAddress();
				if (macAddr != null) {
					StringBuilder sb = new StringBuilder();
					for (int i = 0; i < macAddr.length; i++) {
						sb.append(String.format("%02X%s", macAddr[i], (i < macAddr.length - 1) ? "-" : ""));
					}
					device.macAddress = sb.toString();
				} else {
					device.macAddress = "";
				}
				device.addresses_IPv4 = addr4.toArray(new String[] {});
				device.addresses_IPv6 = addr6.toArray(new String[] {});
				device.flagUp = iface.isUp();
				device.flagLoopback = iface.isLoopback();
				devices.add(device);
			}
		} catch (Throwable ex) {
			Log.e("Network", ex.toString());
		}
		return devices.toArray(new NetworkDevice[0]);
	}

}
