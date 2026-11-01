package com.blackopsarm.dev;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.SystemClock;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;
import org.json.JSONObject;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/** Main app process relay: the SDL Activity stays alive until its native quit completes. */
public final class EngineTransitionService extends Service {
    private static final String TAG = "BlackOpsARM";
    private Thread worker;
    static void request(Context context, Intent next, int pid) {
        Intent request = new Intent(context, EngineTransitionService.class).putExtra("enginePid", pid);
        if (next != null) request.putExtra("next", next);
        if (context.startService(request) == null) throw new IllegalStateException("Engine transition service unavailable");
    }
    @Override public IBinder onBind(Intent intent) { return null; }
    @Override public int onStartCommand(Intent request, int flags, int startId) {
        if (request == null || (worker != null && worker.isAlive())) return START_NOT_STICKY;
        final int pid = request.getIntExtra("enginePid", -1);
        final Intent next = request.getParcelableExtra("next");
        if (pid <= 0 || pid == android.os.Process.myPid() || (next != null &&
                (next.getComponent() == null || !getPackageName().equals(next.getComponent().getPackageName()) ||
                !(LauncherActivity.class.getName().equals(next.getComponent().getClassName()) ||
                  AssetImportActivity.class.getName().equals(next.getComponent().getClassName()))))) {
            Log.e(TAG, "Rejected invalid engine transition"); stopSelf(startId); return START_NOT_STICKY;
        }
        worker = new Thread(() -> {
            final long start = SystemClock.elapsedRealtime();
            boolean exited = false, clean = false, replacementRequested = false;
            String failure = "";
            try {
                while (SystemClock.elapsedRealtime() - start < 30000) {
                    try { Os.kill(pid, 0); }
                    catch (ErrnoException error) {
                        if (error.errno != OsConstants.ESRCH) throw error;
                        exited = true; break;
                    }
                    Thread.sleep(50);
                }
                if (!exited) throw new IllegalStateException("The engine did not finish shutting down");
                File user = new File(getFilesDir(), "user");
                try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                        new FileInputStream(new File(user, ".blackopsarm-instance")), StandardCharsets.UTF_8))) {
                    clean = ("clean pid=" + pid).equals(reader.readLine());
                }
                if (!clean) Log.w(TAG, "Previous engine exited without a clean marker; retaining the last disk save");
                if (next != null) {
                    next.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                    startActivity(next); replacementRequested = true;
                }
            } catch (Exception error) {
                failure = error.toString(); Log.e(TAG, "Engine transition failed", error);
            } finally {
                try {
                    JSONObject record = new JSONObject().put("enginePid", pid).put("processExited", exited)
                            .put("cleanExit", clean).put("replacementRequested", replacementRequested)
                            .put("target", next == null ? "quit" : next.getComponent().getClassName())
                            .put("elapsedMs", SystemClock.elapsedRealtime() - start).put("error", failure);
                    File file = new File(new File(getFilesDir(), "user"), "engine-transition.json");
                    try (FileOutputStream output = new FileOutputStream(file)) {
                        output.write(record.toString().getBytes(StandardCharsets.UTF_8)); output.getFD().sync();
                    }
                    Log.i(TAG, "ENGINE TRANSITION " + record);
                } catch (Exception error) { Log.e(TAG, "Engine transition result could not be recorded", error); }
                stopSelf(startId);
            }
        }, "engine-transition");
        worker.start();
        return START_NOT_STICKY;
    }
}
