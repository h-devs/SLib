package io.slib.examples.phonecall;

public class CallService extends slib.platform.android.call.CallService {

	public CallService() {
		setCallActivityClass(MainActivity.class);
	}

}
