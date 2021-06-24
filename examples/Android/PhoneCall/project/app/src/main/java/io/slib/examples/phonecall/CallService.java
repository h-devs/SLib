package io.slib.examples.phonecall;

public class CallService extends slib.android.call.CallService {

	public CallService() {
		setCallActivityClass(MainActivity.class);
	}

}
