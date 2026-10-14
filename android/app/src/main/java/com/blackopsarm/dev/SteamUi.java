package com.blackopsarm.dev;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Application;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/** Real Activity UI service. This does not configure SDL or publish engine readiness. */
public final class SteamUi {
    private static final String TAG = "BlackOpsARM-Steam";
    private static final int DISPATCH_SECONDS = 30;
    private SteamUi() {}
    private static boolean alive(Activity activity) {
        return activity != null && !activity.isFinishing() && !activity.isDestroyed();
    }
    private static boolean launch(Activity activity, String value) {
        if (!alive(activity) || value == null) return false;
        try {
            Uri uri = Uri.parse(value);
            if (!"steam".equals(uri.getScheme())) return false;
            activity.startActivity(new Intent(Intent.ACTION_VIEW, uri));
            return true; // Actual Android startActivity returned; not remote Steam readiness.
        } catch (ActivityNotFoundException | SecurityException error) {
            Log.e(TAG, "Steam intent launch failed", error);
            return false;
        } catch (RuntimeException error) {
            Log.e(TAG, "Invalid Steam intent or Activity state", error);
            return false;
        }
    }
    public static boolean launchSteam(Activity activity, String uri) {
        if (!alive(activity)) return false;
        if (Looper.myLooper() == Looper.getMainLooper()) return launch(activity, uri);
        CountDownLatch complete = new CountDownLatch(1);
        AtomicBoolean cancelled = new AtomicBoolean();
        AtomicBoolean result = new AtomicBoolean();
        Object dispatch = new Object();
        Handler ui = new Handler(Looper.getMainLooper());
        if (!ui.post(() -> {
            synchronized (dispatch) {
                try { if (!cancelled.get()) result.set(launch(activity, uri)); }
                finally { complete.countDown(); }
            }
        })) return false;
        try {
            if (!complete.await(DISPATCH_SECONDS, TimeUnit.SECONDS)) {
                synchronized (dispatch) {
                    if (complete.getCount() == 0) return result.get();
                    cancelled.set(true);
                }
                Log.e(TAG, "UI launch dispatch timed out before launch");
                return false;
            }
            return result.get();
        } catch (InterruptedException error) {
            Thread.currentThread().interrupt();
            synchronized (dispatch) {
                if (complete.getCount() == 0) return result.get();
                cancelled.set(true);
            }
            Log.e(TAG, "UI launch wait interrupted before launch", error);
            return false;
        }
    }
    private static final class DialogRequest implements Application.ActivityLifecycleCallbacks {
        final Activity activity;
        final String title, message;
        final Handler ui = new Handler(Looper.getMainLooper());
        final CountDownLatch shown = new CountDownLatch(1), dismissed = new CountDownLatch(1);
        final AtomicBoolean cancelled = new AtomicBoolean();
        final AtomicBoolean finished = new AtomicBoolean();
        volatile boolean visible, acknowledged;
        AlertDialog dialog;
        boolean registered;
        DialogRequest(Activity activity, String title, String message) {
            this.activity = activity; this.title = title; this.message = message;
        }
        void finish(boolean userDismissed) {
            if (!finished.compareAndSet(false, true)) return;
            acknowledged = userDismissed;
            try {
                if (registered) {
                    activity.getApplication().unregisterActivityLifecycleCallbacks(this);
                    registered = false;
                }
            } catch (RuntimeException error) { Log.e(TAG, "Activity observer retirement failed", error); }
            try { if (dialog != null && dialog.isShowing()) dialog.dismiss(); }
            catch (RuntimeException error) { Log.e(TAG, "Dialog retirement failed", error); }
            finally { shown.countDown(); dismissed.countDown(); }
        }
        void show() {
            if (cancelled.get() || !alive(activity)) { finish(false); return; }
            try {
                activity.getApplication().registerActivityLifecycleCallbacks(this);
                registered = true;
                dialog = new AlertDialog.Builder(activity).setTitle(title).setMessage(message)
                    .setPositiveButton(android.R.string.ok, (ignored, button) -> {})
                    .create();
                dialog.setOnDismissListener(ignored -> finish(visible && !cancelled.get() && alive(activity)));
                dialog.show();
                visible = dialog.isShowing();
                if (!visible) { finish(false); return; }
                shown.countDown();
            } catch (RuntimeException error) {
                Log.e(TAG, "Steam error dialog failed", error); finish(false);
            }
        }
        void cancel() {
            cancelled.set(true);
            if (!ui.post(() -> finish(false))) Log.e(TAG, "UI dialog cancellation dispatch failed");
        }
        public void onActivityDestroyed(Activity other) { if (other == activity) finish(false); }
        public void onActivityCreated(Activity a, Bundle b) {}
        public void onActivityStarted(Activity a) {}
        public void onActivityResumed(Activity a) {}
        public void onActivityPaused(Activity a) {}
        public void onActivityStopped(Activity a) {}
        public void onActivitySaveInstanceState(Activity a, Bundle b) {}
    }
    public static boolean showErrorAndWait(Activity activity, String title, String message) {
        // The native caller quits immediately afterward: never block Android's UI looper.
        if (Looper.myLooper() == Looper.getMainLooper()) {
            Log.e(TAG, "Steam dialog wait rejected on UI thread"); return false;
        }
        if (!alive(activity) || title == null || message == null) return false;
        DialogRequest request = new DialogRequest(activity, title, message);
        if (!request.ui.post(request::show)) return false;
        try {
            if (!request.shown.await(DISPATCH_SECONDS, TimeUnit.SECONDS)) {
                request.cancel(); Log.e(TAG, "UI dialog dispatch timed out"); return false;
            }
            // User dismissal or real Activity destruction releases this worker. No synthetic ack.
            request.dismissed.await();
            return request.acknowledged;
        } catch (InterruptedException error) {
            request.cancel(); Thread.currentThread().interrupt();
            Log.e(TAG, "Dialog wait interrupted", error); return false;
        }
    }
}
