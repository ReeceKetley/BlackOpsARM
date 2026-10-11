package com.blackopsarm.dev;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

/** Packaging and JNI environment check only; this does not start the engine. */
public final class DevelopmentActivity extends Activity {
    static {
        System.loadLibrary("blackopsarm_environment");
    }

    private static native String nativeEnvironment();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView status = new TextView(this);
        status.setText("BlackOpsARM development environment\n\n"
                + nativeEnvironment()
                + "\n\nEngine and renderer are not connected yet.");
        status.setTextSize(20);
        int padding = (int) (24 * getResources().getDisplayMetrics().density);
        status.setPadding(padding, padding, padding, padding);
        setContentView(status);
    }
}
