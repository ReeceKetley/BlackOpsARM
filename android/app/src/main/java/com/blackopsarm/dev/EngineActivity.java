package com.blackopsarm.dev;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.graphics.Color;
import android.graphics.Typeface;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import java.util.Locale;
import android.util.Log;
import android.widget.Toast;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import java.io.File;
import java.io.IOException;
import org.libsdl.app.SDLActivity;

/** Real matching SDLActivity bootstrap, launched with committed app-owned asset paths. */
public final class EngineActivity extends SDLActivity {
    public static final String EXTRA_GAME_ROOT = "com.blackopsarm.dev.GAME_ROOT";
    public static final String EXTRA_USER_ROOT = "com.blackopsarm.dev.USER_ROOT";
    // Launcher start: map zone stem, and on-screen control opacity (0.2..1) / size (0.7..1.4).
    public static final String EXTRA_LAUNCHER_MAP = "com.blackopsarm.dev.LAUNCHER_MAP";
    public static final String EXTRA_TOUCH_OPACITY = "com.blackopsarm.dev.TOUCH_OPACITY";
    public static final String EXTRA_TOUCH_SCALE = "com.blackopsarm.dev.TOUCH_SCALE";
    private boolean adopted;
    private String gameRoot;
    private String userRoot;
    private AssetImportStore.UseLock assetUse;
    private native boolean nativeAdoptSDLActivity();
    private native boolean nativeSetPerformanceOverlayEnabled(boolean enabled);
    private native boolean nativeReadPerformanceOverlay(double[] values);
    private final Handler performanceHandler = new Handler(Looper.getMainLooper());
    private final double[] performanceValues = new double[13];
    private TextView performanceView;
    private boolean performanceVisible;
    private boolean performanceResumed;
    private boolean performanceNativeAvailable = true;
    private boolean performanceSurfaceReady;
    private final SurfaceHolder.Callback performanceSurface = new SurfaceHolder.Callback() {
        @Override public void surfaceCreated(SurfaceHolder holder) { /* Positive size arrives in surfaceChanged. */ }
        @Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            performanceSurfaceReady = width > 0 && height > 0 && holder.getSurface().isValid();
            updatePerformanceOverlay();
            updateTouchAvailability();
        }
        @Override public void surfaceDestroyed(SurfaceHolder holder) {
            performanceSurfaceReady = false;
            updatePerformanceOverlay();
            updateTouchAvailability();
        }
    };
    private final Runnable performanceRefresh = new Runnable() {
        @Override public void run() {
            if (!performanceVisible || !performanceResumed || performanceView == null) return;
            try {
                if (!nativeReadPerformanceOverlay(performanceValues)) {
                    performanceView.setText("Performance unavailable | Vol+ toggles");
                } else {
                    double[] v = performanceValues;
                    String render = fresh(v[6], 1.5)
                            ? String.format(Locale.US, "Render %.1f FPS | %.2f ms interval", v[0], v[1])
                            : unavailable("Render FPS", v[6]);
                    String cpu = fresh(v[8], 2.0) && v[4] >= 0
                            ? String.format(Locale.US, "CPU process %.1f%% (100%% = 1 core)", v[4])
                            : unavailable("CPU process", v[8]);
                    String ram = fresh(v[9], 3.0) && v[5] >= 0
                            ? String.format(Locale.US, "Engine RSS %.1f MiB", v[5])
                            : unavailable("Engine RSS", v[9]);
                    String frameCpu = "CPU main " + measured(v[3], v[7])
                            + " / Com_Frame | render " + measured(v[2], v[6]) + " / submission";
                    String presentationLabel = getIntent().getBooleanExtra("nativeVulkanPresentation", false)
                            ? "Native presentation check (no game frame)\n" : "";
                    performanceView.setText(presentationLabel + render + "\n" + cpu + "\n" + frameCpu + "\n"
                            + ram + " | " + gpu(v[10], (int)v[11], v[12]) + "\nSubmission rate; Vol+ toggles");
                }
            } catch (LinkageError | RuntimeException failure) {
                performanceNativeAvailable = false;
                try { nativeSetPerformanceOverlayEnabled(false); }
                catch (LinkageError ignored) { /* Native interface is unavailable. */ }
                performanceView.setText("Performance unavailable | Vol+ toggles");
                Log.e("BlackOpsARM", "Performance snapshot unavailable", failure);
                return;
            }
            performanceHandler.postDelayed(this, 500);
        }
    };

    private static String gpu(double ms, int status, double age) {
        if (status == 2) {
            return ms >= 0 && fresh(age, 1.5)
                    ? String.format(Locale.US, "GPU span %.2f ms (%.1f s old)", ms, age)
                    : unavailable("GPU", age);
        }
        String state = switch (status) {
            case 1 -> "pending";
            case 3 -> "unsupported";
            case 4 -> "query failed";
            case 5 -> "invalid/disjoint";
            default -> "unavailable";
        };
        return age >= 0 ? String.format(Locale.US, "GPU %s (%.1f s)", state, age) : "GPU " + state;
    }
    private static boolean fresh(double age, double limit) { return age >= 0 && age <= limit; }
    private static String unavailable(String label, double age) {
        return age >= 0 ? String.format(Locale.US, "%s stale (%.1f s)", label, age)
                : label + " pending/unavailable";
    }
    private static String measured(double value, double age) {
        return value >= 0 && fresh(age, 1.5) ? String.format(Locale.US, "%.2f ms", value) : "n/a";
    }
    private void createPerformanceOverlay() {
        performanceVisible = getPreferences(MODE_PRIVATE).getBoolean("performanceOverlay", true);
        performanceSurfaceReady = mSurface != null && mSurface.mIsSurfaceReady
                && mSurface.getWidth() > 0 && mSurface.getHeight() > 0 && mSurface.getHolder().getSurface().isValid();
        mSurface.getHolder().addCallback(performanceSurface);
        performanceView = new TextView(this);
        performanceView.setTextColor(Color.WHITE);
        performanceView.setBackgroundColor(0xB0000000);
        performanceView.setTypeface(Typeface.MONOSPACE);
        performanceView.setTextSize(12);
        int pad = Math.round(6 * getResources().getDisplayMetrics().density);
        performanceView.setPadding(pad, pad, pad, pad);
        performanceView.setFocusable(false);
        performanceView.setClickable(false);
        performanceView.setLongClickable(false);
        performanceView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        params.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        params.addRule(RelativeLayout.ALIGN_PARENT_START);
        mLayout.addView(performanceView, params);
        updatePerformanceOverlay();
    }
    private void updatePerformanceOverlay() {
        performanceHandler.removeCallbacks(performanceRefresh);
        boolean enabled = adopted && !overlayShown && performanceVisible && performanceResumed && performanceSurfaceReady
                && mSurface != null && mSurface.mIsSurfaceReady && mSurface.getHolder().getSurface().isValid()
                && performanceNativeAvailable;
        if (performanceView != null) performanceView.setVisibility(enabled ? View.VISIBLE : View.GONE);
        try {
            if (adopted && !nativeSetPerformanceOverlayEnabled(enabled)) {
                performanceNativeAvailable = false;
                if (performanceView != null && enabled) performanceView.setText("Performance sampler unavailable");
                return;
            }
        } catch (LinkageError failure) {
            performanceNativeAvailable = false;
            Log.e("BlackOpsARM", "Performance native interface unavailable", failure);
            if (performanceView != null && enabled) performanceView.setText("Performance unavailable");
            return;
        }
        if (enabled) performanceHandler.post(performanceRefresh);
    }
    private native void nativeSetTouchControlsEnabled(boolean enabled);
    private native boolean nativeReadTouchState(int[] state);
    private native boolean nativeTouchButton(int epoch, int action, boolean down, float x, float y);
    private native boolean nativeTouchMotion(int epoch, boolean look, float x, float y);
    private TouchControls touchControls;
    private boolean touchVisible;
    private boolean touchWindowFocused;
    private boolean touchAdmission;
    private boolean touchNativeAvailable = true;
    private final int[] touchState = new int[3];
    private final Runnable touchRefresh = new Runnable() {
        @Override public void run() {
            if (!touchAdmission || touchControls == null) return;
            try {
                if (!nativeReadTouchState(touchState)) throw new IllegalStateException("Touch snapshot unavailable");
                touchControls.readState(touchState[0], touchState[1] != 0, touchState[2] != 0);
            } catch (LinkageError | RuntimeException failure) {
                touchNativeAvailable = false;
                updateTouchAvailability();
                Log.e("BlackOpsARM", "Touch input unavailable", failure);
                return;
            }
            performanceHandler.postDelayed(this, 100);
        }
    };
    // ---- in-process launcher menu --------------------------------------------------
    private native int nativeReadSessionState();
    private native boolean nativeRequestMapStart(String map);
    private native int nativeReadLoadProgress();
    private native void nativeSetLoadZones(String csv);
    private int lastProgress = -1, progressMax;
    private volatile long expectedLoadBytes;
    private WebView menu;
    private boolean overlayShown, menuMode, matchSeen;
    private int sessionState = -1, runningHeight = 540;
    private float touchOpacity = 1f, touchScale = 1f;
    private LauncherBridge launcherBridge;
    private final Runnable sessionPoll = new Runnable() {
        @Override public void run() {
            try {
                int state = nativeReadSessionState();
                if (state != sessionState) { sessionState = state; onSessionState(state); }
                if (menu != null && state != 3) pushProgress();
            } catch (LinkageError failure) { Log.e("BlackOpsARM", "Session state unavailable", failure); return; }
            performanceHandler.postDelayed(this, 250);
        }
    };
    private final LauncherBridge.Host menuHost = new LauncherBridge.Host() {
        @Override public File gameRoot() { return new File(EngineActivity.this.gameRoot); }
        @Override public File userRoot() { return new File(EngineActivity.this.userRoot); }
        @Override public int runningHeight() { return runningHeight; }
        @Override public boolean needsFreshEngine() { return matchSeen; }
        @Override public boolean requestMap(String map) { prepareProgress(map); return nativeRequestMapStart(map); }
        @Override public void applyTouch(float opacity, float scale) {
            runOnUiThread(() -> {
                touchOpacity = opacity; touchScale = scale;
                getSharedPreferences("launcher", MODE_PRIVATE).edit().putFloat("touchOpacity", opacity).putFloat("touchScale", scale).apply();
                if (touchControls != null) touchControls.applyLook();
            });
        }
        @Override public void rememberHeight(int height) {
            getSharedPreferences("launcher", MODE_PRIVATE).edit().putInt("height", height).apply();
        }
        @Override public void restartWithMap(String map, int height) {
            // The render size is fixed for the life of an engine process: start a fresh one.
            runOnUiThread(() -> relaunch(new Intent(EngineActivity.this, LauncherActivity.class)
                    .putExtra("launchMap", map).putExtra("launchHeight", height)));
        }
        @Override public void openImporter() {
            runOnUiThread(() -> relaunch(new Intent(EngineActivity.this, AssetImportActivity.class)));
        }
        @Override public void quit() {
            runOnUiThread(() -> { finishAndRemoveTask(); killEngineSoon(); });
        }
    };
    /** Starts another activity in a clean task, then ends this engine process (its state cannot be reset). */
    private void relaunch(Intent next) {
        next.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(next);
        killEngineSoon();
    }
    private void killEngineSoon() {
        performanceHandler.postDelayed(() -> android.os.Process.killProcess(android.os.Process.myPid()), 300);
    }
    @SuppressLint("SetJavaScriptEnabled")
    private void showMenu(String hash) {
        if (menu != null || isFinishing() || mLayout == null) return;
        WebView view = new WebView(this);
        view.setBackgroundColor(Color.BLACK);
        WebSettings settings = view.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true); // the page keeps its settings in localStorage
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setAllowContentAccess(false);
        settings.setAllowFileAccessFromFileURLs(true);
        view.setWebViewClient(new WebViewClient() {
            @Override public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest request) {
                return !request.getUrl().toString().startsWith("file:///android_asset/launcher/");
            }
        });
        view.addJavascriptInterface(launcherBridge, "BlackOpsARM");
        mLayout.addView(view, new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        menu = view;
        overlayShown = true;
        view.loadUrl("file:///android_asset/launcher/index.html" + hash);
        updatePerformanceOverlay();
        updateTouchAvailability();
    }
    /** Frees the page, its audio and its renderer completely: nothing of the menu runs during play. */
    private void hideMenu() {
        if (menu == null) return;
        WebView view = menu;
        menu = null;
        overlayShown = false;
        view.evaluateJavascript("window.onNativeFreeze&&window.onNativeFreeze()", null);
        view.removeJavascriptInterface("BlackOpsARM");
        view.loadUrl("about:blank");
        if (mLayout != null) mLayout.removeView(view);
        view.destroy();
        updatePerformanceOverlay();
        updateTouchAvailability();
    }
    /** Tells the engine which zones this map loads, so their real read progress can be shown. */
    private void prepareProgress(String map) {
        final File root = new File(gameRoot, "zone");
        new Thread(() -> {
            String[] names = {"common_zombie_patch", "en_common_zombie", "common_zombie", map + "_patch", "en_" + map, map};
            long total = 0;
            for (String name : names) {
                for (String dir : new String[] {"common", "English", "english"}) {
                    File zone = new File(root, dir + "/" + name + ".ff");
                    if (zone.isFile()) { total += zone.length(); break; }
                }
            }
            try { nativeSetLoadZones(String.join(",", names)); } catch (LinkageError failure) { return; }
            expectedLoadBytes = total;
        }, "load-progress-zones").start();
    }
    /** Pushes the engine's real fastfile loading fraction to the page (never moving backwards: each zone restarts it). */
    private void pushProgress() {
        long expected = expectedLoadBytes;
        if (expected <= 0) return;
        // Compressed 256 KB reads of this map's zones, against those zones' file sizes.
        long loaded = (long) nativeReadLoadProgress() * 262144L;
        int progress = Math.max(progressMax, (int) Math.min(990L, loaded * 1000L / expected));
        progressMax = progress;
        if (progress != lastProgress && menu != null) {
            lastProgress = progress;
            menu.evaluateJavascript("window.onNativeProgress&&window.onNativeProgress(" + progress + ")", null);
        }
    }
    private void onSessionState(int state) {
        Log.i("BlackOpsARM", "LAUNCHER session state " + state + " menu=" + (menu != null));
        if (state == 3) { matchSeen = true; hideMenu(); return; }
        if (menu == null && matchSeen && !isFinishing()) showMenu("#maps"); // back from a match
        if (menu != null) menu.evaluateJavascript("window.onNativeSession&&window.onNativeSession(" + state + ")", null);
    }
    @Override public void onBackPressed() {
        if (menu != null) { menu.evaluateJavascript("window.onNativeBack&&window.onNativeBack()", null); return; }
        super.onBackPressed();
    }

    private void createTouchControls() {
        touchVisible = getPreferences(MODE_PRIVATE).getBoolean("touchControls", true);
        touchWindowFocused = hasWindowFocus();
        touchControls = new TouchControls();
        touchControls.setAlpha(Math.max(0.2f, Math.min(1f, touchOpacity)));
        mLayout.addView(touchControls, new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        // Metrics stay readable above the transparent control layer.
        if (performanceView != null) performanceView.bringToFront();
        updateTouchAvailability();
    }
    private void updateTouchAvailability() {
        performanceHandler.removeCallbacks(touchRefresh);
        boolean enabled = adopted && !overlayShown && touchVisible && performanceResumed && touchWindowFocused
                && performanceSurfaceReady && mSurface != null && mSurface.mIsSurfaceReady
                && mSurface.getHolder().getSurface().isValid() && touchNativeAvailable;
        if (touchAdmission != enabled) {
            touchAdmission = enabled;
            if (touchControls != null) touchControls.clearPointers();
            try { if (adopted) nativeSetTouchControlsEnabled(enabled); }
            catch (LinkageError failure) {
                touchNativeAvailable = false;
                touchAdmission = false;
                Log.e("BlackOpsARM", "Touch native interface unavailable", failure);
            }
        }
        if (touchControls != null) touchControls.invalidate();
        if (touchAdmission) performanceHandler.post(touchRefresh);
    }
    private void cancelTouchGesture() {
        if (touchControls != null) touchControls.clearPointers();
        // Cancellation clears the native mailbox and invalidates its epoch.
        // Re-admission requires a subsequent real owner frame.
        try { if (adopted && touchNativeAvailable) nativeSetTouchControlsEnabled(false); }
        catch (LinkageError failure) { touchNativeAvailable = false; }
        touchAdmission = false;
        updateTouchAvailability();
    }
    @Override public void onWindowFocusChanged(boolean hasFocus) {
        touchWindowFocused = hasFocus;
        // Cancel before SDL can pause its owner thread.
        updateTouchAvailability();
        super.onWindowFocusChanged(hasFocus);
    }
    private final class TouchControls extends View {
        private final android.graphics.Paint paint = new android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG);
        private final android.util.SparseArray<TouchPointer> pointers = new android.util.SparseArray<>();
        private final int[] refs = new int[16];
        private final android.graphics.RectF toggle = new android.graphics.RectF();
        private final android.graphics.RectF[] actionBounds = new android.graphics.RectF[15];
        private final String[] labels = {"", "", "", "", "FIRE", "AIM", "JUMP", "LOAD", "CROUCH", "USE", "RUN", "SWAP", "MELEE", "FRAG", "MENU"};
        private float unit, moveX, moveY, moveRadius;
        private int epoch, movePointer = -1, lookPointer = -1, cursorPointer = -1;
        private boolean ready, gameplay;
        private final class TouchPointer {
            int role, mask;
            float x, y;
            TouchPointer(int role, float x, float y) { this.role = role; this.x = x; this.y = y; }
        }
        TouchControls() {
            super(EngineActivity.this);
            setFocusable(false);
            setContentDescription("Tablet controls. Left pad moves, drag open space to look, right buttons act. Touch button hides controls.");
            for (int i = 0; i < actionBounds.length; i++) actionBounds[i] = new android.graphics.RectF();
        }
        /** Re-applies the opacity/size chosen in the launcher menu. */
        void applyLook() {
            setAlpha(Math.max(0.2f, Math.min(1f, touchOpacity)));
            onSizeChanged(getWidth(), getHeight(), 0, 0);
            invalidate();
        }
        void clearPointers() {
            pointers.clear();
            java.util.Arrays.fill(refs, 0);
            movePointer = lookPointer = cursorPointer = -1;
            epoch = 0; ready = false;
            invalidate();
        }
        void readState(int newEpoch, boolean available, boolean playing) {
            // Polled every 100 ms. Each redraw re-renders this full-screen
            // layer on the GPU the engine uses, so redraw only on change.
            final boolean changed = epoch != newEpoch || ready != available || gameplay != playing;
            if (epoch != newEpoch || gameplay != playing || (!available && (ready || pointers.size() != 0))) clearPointers();
            epoch = newEpoch; ready = available; gameplay = playing;
            if (changed) invalidate();
        }
        @Override protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            super.onSizeChanged(w, h, oldw, oldh);
            unit = Math.min(getResources().getDisplayMetrics().density,
                    Math.min(w / 640f, h / 360f))
                    * Math.max(0.7f, Math.min(1.4f, touchScale));
            moveRadius = 68 * unit;
            moveX = 90 * unit; moveY = h - 94 * unit;
            toggle.set(w - 102 * unit, 12 * unit, w - 12 * unit, 46 * unit);
            float radius = 24 * unit;
            for (int i = 4; i <= 13; i++) {
                int slot = i - 4;
                float x = w - (42 + (slot % 4) * 56) * unit;
                float y = h - (48 + (slot / 4) * 59) * unit;
                actionBounds[i].set(x - radius, y - radius, x + radius, y + radius);
            }
            // Fire and aim are the largest, highest buttons for two thumbs.
            actionBounds[4].set(w - 80 * unit, h - 244 * unit, w - 12 * unit, h - 176 * unit);
            actionBounds[5].set(w - 158 * unit, h - 236 * unit, w - 98 * unit, h - 176 * unit);
            actionBounds[14].set(w - 198 * unit, 12 * unit, w - 112 * unit, 46 * unit);
            if (oldw > 0 && (w != oldw || h != oldh)) cancelTouchGesture();
        }
        private void text(android.graphics.Canvas canvas, String label, float x, float y, float size) {
            paint.setStyle(android.graphics.Paint.Style.FILL);
            paint.setColor(0xE8FFFFFF); paint.setTextAlign(android.graphics.Paint.Align.CENTER);
            paint.setTypeface(Typeface.DEFAULT_BOLD); paint.setTextSize(size * unit);
            canvas.drawText(label, x, y - (paint.ascent() + paint.descent()) / 2, paint);
        }
        private void button(android.graphics.Canvas canvas, android.graphics.RectF bounds, String label, boolean held) {
            paint.setStyle(android.graphics.Paint.Style.FILL);
            paint.setColor(held ? 0x99677883 : 0x55000000);
            canvas.drawRoundRect(bounds, 10 * unit, 10 * unit, paint);
            paint.setStyle(android.graphics.Paint.Style.STROKE); paint.setStrokeWidth(unit);
            paint.setColor(held ? 0xEEFFFFFF : 0x779BA8AC);
            canvas.drawRoundRect(bounds, 10 * unit, 10 * unit, paint);
            text(canvas, label, bounds.centerX(), bounds.centerY(), 10);
        }
        @Override protected void onDraw(android.graphics.Canvas canvas) {
            super.onDraw(canvas);
            button(canvas, toggle, touchVisible ? "TOUCH ON" : "TOUCH OFF", touchVisible);
            if (!touchVisible) return;
            button(canvas, actionBounds[14], "MENU", refs[14] != 0);
            paint.setStyle(android.graphics.Paint.Style.FILL); paint.setColor(0x40000000);
            canvas.drawCircle(moveX, moveY, moveRadius, paint);
            paint.setStyle(android.graphics.Paint.Style.STROKE); paint.setStrokeWidth(unit); paint.setColor(0x779BA8AC);
            canvas.drawCircle(moveX, moveY, moveRadius, paint);
            text(canvas, "^", moveX, moveY - 44 * unit, 16);
            text(canvas, "v", moveX, moveY + 44 * unit, 16);
            text(canvas, "<", moveX - 44 * unit, moveY, 16);
            text(canvas, ">", moveX + 44 * unit, moveY, 16);
            TouchPointer moving = pointers.get(movePointer);
            float dx = moving == null ? 0 : moving.x - moveX, dy = moving == null ? 0 : moving.y - moveY;
            float length = (float)Math.hypot(dx, dy), limit = moveRadius * .65f;
            if (length > limit) { dx *= limit / length; dy *= limit / length; }
            paint.setStyle(android.graphics.Paint.Style.FILL); paint.setColor(0x889BA8AC);
            canvas.drawCircle(moveX + dx, moveY + dy, 17 * unit, paint);
            if (gameplay) {
                for (int i = 4; i <= 13; i++) button(canvas, actionBounds[i], labels[i], refs[i] != 0);
                text(canvas, "DRAG TO LOOK", getWidth() / 2f, getHeight() - 18 * unit, 10);
            } else {
                button(canvas, actionBounds[6], "OK", refs[6] != 0);
                text(canvas, "TAP MENU / PAD TO SELECT", getWidth() / 2f, getHeight() - 18 * unit, 10);
            }
            if (!ready) text(canvas, touchNativeAvailable ? "TOUCH WAITING" : "TOUCH UNAVAILABLE",
                    getWidth() / 2f, getHeight() - 38 * unit, 10);
        }
        private float nx(float x) { return Math.max(0, Math.min(1, x / Math.max(1, getWidth()))); }
        private float ny(float y) { return Math.max(0, Math.min(1, y / Math.max(1, getHeight()))); }
        private boolean changeMask(TouchPointer pointer, int mask) {
            int changed = pointer.mask ^ mask;
            for (int action = 0; action < refs.length; action++) {
                int bit = 1 << action;
                if ((changed & bit) == 0) continue;
                boolean down = (mask & bit) != 0;
                int count = down ? ++refs[action] : --refs[action];
                if ((down && count == 1) || (!down && count == 0)) {
                    if (!nativeTouchButton(epoch, action, down, nx(pointer.x), ny(pointer.y))) return false;
                }
            }
            pointer.mask = mask;
            return true;
        }
        private boolean move(TouchPointer pointer, float x, float y) {
            float dx = x - pointer.x, dy = y - pointer.y;
            pointer.x = x; pointer.y = y;
            if (pointer.role == -2) {
                float horizontal = (x - moveX) / moveRadius, vertical = (y - moveY) / moveRadius;
                int mask = 0;
                if (vertical < -.22f) mask |= 1;
                if (vertical > .22f) mask |= 2;
                if (horizontal < -.22f) mask |= 4;
                if (horizontal > .22f) mask |= 8;
                return changeMask(pointer, mask);
            }
            if (pointer.role == -3) return nativeTouchMotion(epoch, true,
                    Math.max(-1, Math.min(1, dx / Math.max(1, getWidth()))),
                    Math.max(-1, Math.min(1, dy / Math.max(1, getHeight()))));
            if (pointer.role == 15) return nativeTouchMotion(epoch, false, nx(x), ny(y));
            return true;
        }
        @Override public boolean onTouchEvent(android.view.MotionEvent event) {
            // Keep physical mouse delivery on the existing SDL surface path.
            if (event.isFromSource(android.view.InputDevice.SOURCE_MOUSE)) return false;
            int action = event.getActionMasked(), index = event.getActionIndex();
            float x = event.getX(index), y = event.getY(index);
            if ((action == android.view.MotionEvent.ACTION_DOWN || action == android.view.MotionEvent.ACTION_POINTER_DOWN)
                    && toggle.contains(x, y)) {
                touchVisible = !touchVisible;
                getPreferences(MODE_PRIVATE).edit().putBoolean("touchControls", touchVisible).apply();
                cancelTouchGesture();
                return true;
            }
            if (!touchVisible) return false;
            // All overlay-owned touch streams are consumed, so SDL cannot also
            // translate one of their fingers into a duplicate mouse1 attack.
            if (action == android.view.MotionEvent.ACTION_CANCEL) { cancelTouchGesture(); return true; }
            if (!ready || !touchAdmission) return true;
            boolean accepted = true;
            if (action == android.view.MotionEvent.ACTION_DOWN || action == android.view.MotionEvent.ACTION_POINTER_DOWN) {
                int id = event.getPointerId(index), role = -1;
                for (int i = 4; i <= 14; i++) {
                    if ((gameplay || i == 6 || i == 14) && actionBounds[i].contains(x, y)) { role = i; break; }
                }
                if (role == -1 && movePointer == -1 && Math.hypot(x - moveX, y - moveY) <= moveRadius * 1.2f) {
                    role = -2; movePointer = id;
                } else if (role == -1 && gameplay && lookPointer == -1) { role = -3; lookPointer = id; }
                else if (role == -1 && !gameplay && cursorPointer == -1) { role = 15; cursorPointer = id; }
                TouchPointer pointer = new TouchPointer(role, x, y);
                pointers.put(id, pointer);
                if (role >= 0) accepted = changeMask(pointer, 1 << role);
                else if (role == -2) accepted = move(pointer, x, y);
            } else if (action == android.view.MotionEvent.ACTION_MOVE) {
                for (int i = 0; i < event.getPointerCount() && accepted; i++) {
                    TouchPointer pointer = pointers.get(event.getPointerId(i));
                    if (pointer != null) accepted = move(pointer, event.getX(i), event.getY(i));
                }
            } else if (action == android.view.MotionEvent.ACTION_UP || action == android.view.MotionEvent.ACTION_POINTER_UP) {
                int id = event.getPointerId(index);
                TouchPointer pointer = pointers.get(id);
                if (pointer != null) { pointer.x = x; pointer.y = y; accepted = changeMask(pointer, 0); }
                pointers.remove(id);
                if (movePointer == id) movePointer = -1;
                if (lookPointer == id) lookPointer = -1;
                if (cursorPointer == id) cursorPointer = -1;
            }
            if (!accepted) cancelTouchGesture();
            invalidate();
            return true;
        }
    }

    @Override public boolean dispatchKeyEvent(KeyEvent event) {
        if (adopted && getIntent().getBooleanExtra("nativeVulkanPresentation", false)
                && (event.getKeyCode() == KeyEvent.KEYCODE_BACK || event.getKeyCode() == KeyEvent.KEYCODE_ESCAPE)) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) SDLActivity.nativeSendQuit();
            return true;
        }
        if (adopted && event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) {
                performanceVisible = !performanceVisible;
                getPreferences(MODE_PRIVATE).edit().putBoolean("performanceOverlay", performanceVisible).apply();
                updatePerformanceOverlay();
                Toast.makeText(this, performanceVisible ? "Performance overlay on" : "Performance overlay off",
                        Toast.LENGTH_SHORT).show();
            }
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override protected String[] getLibraries() {
        return new String[] {"SDL2", "blackopsarm_engine"};
    }

    @Override protected String[] getArguments() {
        if (gameRoot == null || userRoot == null || assetUse == null) {
            throw new IllegalStateException("Native launch requires committed, app-accessible game files");
        }
        // The matching SDL SDK prepends its library argument: SDL_main receives argc=4.
        if (getIntent().getBooleanExtra("nativeVulkanBackend", false)
                && !getIntent().getBooleanExtra("nativeVulkanPresentation", false)) {
            String launcherMap = getIntent().getStringExtra(EXTRA_LAUNCHER_MAP);
            return new String[] {gameRoot, userRoot, "--native-vulkan",
                    Integer.toString(getIntent().getIntExtra("nativeVulkanHeight", 0)),
                    launcherMap != null ? "--map=" + launcherMap
                            : getIntent().getBooleanExtra("nativeAutoKino", true) ? "--kino" : "--menu"};
        }
        if (getIntent().getBooleanExtra("nativeVulkanPresentation", false)) {
            return new String[] {gameRoot, userRoot, "--vulkan-presentation"};
        }
        return new String[] {gameRoot, userRoot, "--rendered",
                getIntent().getBooleanExtra("nativeAutoKino", true) ? "--kino" : "--menu"};
    }

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        try {
            AssetImportStore store = new AssetImportStore(this);
            // A previous engine process may still be shutting down: wait for its lock.
            assetUse = store.acquireWithin(3000);
            File game = store.gameRoot();
            File user = store.userRoot();
            String requestedGame = getIntent().getStringExtra(EXTRA_GAME_ROOT);
            String requestedUser = getIntent().getStringExtra(EXTRA_USER_ROOT);
            if (requestedGame == null || requestedUser == null
                    || !game.equals(new File(requestedGame).getCanonicalFile())
                    || !user.equals(new File(requestedUser).getCanonicalFile())) {
                throw new IOException("Launch the game from the game folder screen.");
            }
            store.requireCommitted(game);
            gameRoot = game.getPath();
            userRoot = user.getPath();
        } catch (IOException | RuntimeException failure) {
            Log.e("BlackOpsARM", "Game storage unavailable", failure);
            Toast.makeText(this, failure.getMessage() == null ? "Game storage is unavailable. Import again."
                    : failure.getMessage(), Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        // The SDK has now set up JNI, its actual singleton/context/surface and
        // controller handler. Never call SDL.setupJNI a second time here.
        try {
            adopted = nativeAdoptSDLActivity();
        } catch (LinkageError error) {
            Log.e("BlackOpsARM", "Engine bootstrap native library unavailable", error);
        }
        if (!adopted) {
            Log.e("BlackOpsARM", "Actual SDLActivity adoption failed");
            Toast.makeText(this, "The native engine could not start. See the development log.", Toast.LENGTH_LONG).show();
            finish();
        } else {
            SharedPreferences launcherPrefs = getSharedPreferences("launcher", MODE_PRIVATE);
            runningHeight = getIntent().getIntExtra("nativeVulkanHeight", 540);
            touchOpacity = launcherPrefs.getFloat("touchOpacity", 1f);
            touchScale = launcherPrefs.getFloat("touchScale", 1f);
            launcherBridge = new LauncherBridge(menuHost);
            createPerformanceOverlay();
            if (!getIntent().getBooleanExtra("nativeVulkanPresentation", false)) createTouchControls();
            // Normal start (the menu): the page covers the engine while it boots to its hidden
            // frontend. A restart for a chosen map shows the page's loading screen instead; the
            // testing shortcut (nativeAutoKino) starts without it.
            menuMode = !getIntent().getBooleanExtra("nativeAutoKino", true);
            String launchMap = getIntent().getStringExtra(EXTRA_LAUNCHER_MAP);
            if (launchMap != null) { prepareProgress(launchMap); showMenu("#loading=" + launchMap); }
            else if (menuMode) showMenu("");
            performanceHandler.post(sessionPoll);
        }
    }

    @Override protected void onResume() {
        super.onResume();
        if (menu != null) { menu.onResume(); menu.resumeTimers(); }
        performanceResumed = true;
        updatePerformanceOverlay();
        updateTouchAvailability();
    }

    @Override protected void onPause() {
        if (menu != null) { menu.onPause(); menu.pauseTimers(); }
        performanceResumed = false;
        updatePerformanceOverlay();
        updateTouchAvailability();
        super.onPause();
    }

    @Override protected void resumeNativeThread() {
        // SDK 2.30.11 uses onStart as well as onResume (multi-window on API 24+).
        // Guard the actual SDK thread-resume hook while satisfying Activity lifecycle.
        if (adopted) super.resumeNativeThread();
    }

    @Override protected void onDestroy() {
        performanceHandler.removeCallbacks(sessionPoll);
        hideMenu();
        performanceResumed = false;
        updatePerformanceOverlay();
        updateTouchAvailability();
        if (mSurface != null) mSurface.getHolder().removeCallback(performanceSurface);
        if (performanceView != null) {
            mLayout.removeView(performanceView);
            performanceView = null;
        }
        if (touchControls != null) {
            mLayout.removeView(touchControls);
            touchControls = null;
        }
        try { super.onDestroy(); }
        finally {
            if (assetUse != null) {
                try { assetUse.close(); }
                catch (IOException failure) { Log.e("BlackOpsARM", "Cannot release game storage", failure); }
                assetUse = null;
            }
        }
    }
}
