package com.blackopsarm.dev;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/** Start screen: the bundled HTML launcher (assets/launcher) chooses a map and settings. */
public final class LauncherActivity extends Activity {
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    // Zone file stems the launcher can start; any other value is rejected.
    private static final String[] MAPS = {"zombie_theater", "zombie_pentagon", "zombie_cosmodrome", "zombie_coast",
            "zombie_temple", "zombie_moon", "zombie_cod5_prototype", "zombie_cod5_asylum", "zombie_cod5_sumpf",
            "zombie_cod5_factory"};
    private WebView web;
    private Bridge bridge;

    @SuppressLint("SetJavaScriptEnabled")
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        web = new WebView(this);
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true); // the page keeps its settings in localStorage
        settings.setMediaPlaybackRequiresUserGesture(true);
        settings.setAllowContentAccess(false);
        // The page uploads its bundled map images to WebGL; asset pages share
        // one file origin only with this (no network or external files are read).
        settings.setAllowFileAccessFromFileURLs(true);
        web.setWebViewClient(new WebViewClient() {
            // The page is local; never navigate this view elsewhere.
            @Override public boolean shouldOverrideUrlLoading(WebView view, android.webkit.WebResourceRequest request) {
                return !request.getUrl().toString().startsWith("file:///android_asset/launcher/");
            }
        });
        bridge = new Bridge();
        web.addJavascriptInterface(bridge, "BlackOpsARM");
        setContentView(web);
        web.loadUrl("file:///android_asset/launcher/index.html");
        autoLaunch(getIntent());
    }

    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        autoLaunch(intent);
    }

    /** Debug builds only: automated map tests start a map from adb
     *  (am start -n .../.LauncherActivity --es autoMap zombie_theater [--es autoConfig "..."]). */
    private void autoLaunch(Intent intent) {
        if ((getApplicationInfo().flags & android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE) == 0) return;
        String map = intent.getStringExtra("autoMap");
        if (map == null) return;
        intent.removeExtra("autoMap");
        String config = intent.getStringExtra("autoConfig");
        if (config == null) config = AUTO_CONFIG;
        try {
            JSONObject request = new JSONObject();
            request.put("map", map);
            request.put("height", intent.getIntExtra("autoHeight", 540));
            request.put("config", config);
            final String json = request.toString();
            new Thread(() -> {
                String error = bridge.launch(json);
                Log.i("BlackOpsARM", "LAUNCHER auto start " + map + (error.isEmpty() ? " ok" : " failed: " + error));
            }, "launcher-auto-start").start();
        } catch (JSONException failure) {
            Log.e("BlackOpsARM", "LAUNCHER auto start request failed", failure);
        }
    }

    // The tested settings (540p, SGSR, 30 FPS, medium textures, no shadows) plus god mode.
    private static final String AUTO_CONFIG = String.join(System.lineSeparator(),
            "seta r_picmip_manual 1", "seta r_picmip 1", "seta r_picmip_bump 1", "seta r_picmip_spec 1",
            "seta r_lodScaleRigid 1", "seta r_lodScaleSkinned 1", "seta com_maxfps 30", "set r_nativeSgsr 1", "seta sm_enable 0",
            "set r_nativeBloom 0", "seta r_dof_enable 0", "seta fx_marks 0", "set r_nativeParallelRecord 2", "set bo1_testclient_god 1");

    @Override public void onWindowFocusChanged(boolean focused) {
        super.onWindowFocusChanged(focused);
        if (focused && android.os.Build.VERSION.SDK_INT >= 30) {
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else if (focused) {
            web.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }

    @Override protected void onResume() {
        super.onResume();
        if (web != null) {
            web.onResume();
            web.resumeTimers();
            // Back from the importer or a game: let the page re-read the install state.
            web.evaluateJavascript("window.onNativeResume&&window.onNativeResume()", null);
        }
    }

    @Override protected void onPause() {
        // Stop the animated menu while the game (or importer) is in front, so it
        // takes no CPU or GPU time from the engine.
        if (web != null) { web.onPause(); web.pauseTimers(); }
        super.onPause();
    }

    @Override protected void onDestroy() {
        if (web != null) { web.removeJavascriptInterface("BlackOpsARM"); web.destroy(); web = null; }
        super.onDestroy();
    }

    private static boolean knownMap(String map) {
        for (String known : MAPS) if (known.equals(map)) return true;
        return false;
    }

    /** Called by the page on a WebView binder thread; UI work is posted to the main thread. */
    private final class Bridge {
        /** {"installed":bool,"maps":[stems with their zone files present],"problem":text|null} */
        @JavascriptInterface public String status() {
            JSONObject result = new JSONObject();
            try {
                AssetImportStore store = new AssetImportStore(getApplicationContext());
                JSONArray maps = new JSONArray();
                boolean installed;
                String problem = null;
                try (AssetImportStore.UseLock ignored = store.acquire()) {
                    File game = store.gameRoot();
                    try { store.requireCommitted(game); installed = true; }
                    catch (IOException missing) { installed = false; problem = missing.getMessage(); }
                    if (installed) for (String map : MAPS) {
                        if (new File(game, "zone/common/" + map + ".ff").isFile()) maps.put(map);
                    }
                }
                result.put("installed", installed);
                result.put("maps", maps);
                result.put("problem", problem == null ? JSONObject.NULL : problem);
            } catch (IOException | RuntimeException | JSONException failure) {
                try { result.put("installed", false); result.put("maps", new JSONArray()); result.put("problem", String.valueOf(failure.getMessage())); }
                catch (JSONException ignored) { /* Unreachable for these values. */ }
            }
            return result.toString();
        }

        /** {"map":stem,"height":480|540|720,"config":"console lines","touchOpacity":0..1,"touchScale":0.7..1.4} */
        @JavascriptInterface public String launch(String json) {
            try {
                JSONObject request = new JSONObject(json);
                String map = request.getString("map");
                int height = request.getInt("height");
                String config = request.getString("config");
                if (!knownMap(map)) return "Unknown map";
                if (height != 480 && height != 540 && height != 720) return "Unsupported resolution";
                if (config.length() > 16384 || config.indexOf('\0') >= 0) return "Invalid settings";
                float opacity = (float) Math.max(0.2, Math.min(1.0, request.optDouble("touchOpacity", 1.0)));
                float scale = (float) Math.max(0.7, Math.min(1.4, request.optDouble("touchScale", 1.0)));
                AssetImportStore store = new AssetImportStore(getApplicationContext());
                String gamePath, userPath;
                try (AssetImportStore.UseLock ignored = store.acquire()) {
                    File game = store.gameRoot();
                    store.requireCommitted(game);
                    if (!new File(game, "zone/common/" + map + ".ff").isFile()) return "This map's files are not installed";
                    File user = store.userRoot();
                    File temporary = new File(user, "launcher.cfg.tmp");
                    try (FileOutputStream out = new FileOutputStream(temporary)) {
                        out.write(("// written by the launcher before each start\n" + config + "\n").getBytes(StandardCharsets.UTF_8));
                        out.getFD().sync();
                    }
                    if (!temporary.renameTo(new File(user, "launcher.cfg"))) throw new IOException("Could not save launcher settings");
                    gamePath = game.getPath();
                    userPath = user.getPath();
                }
                final Intent engine = new Intent(LauncherActivity.this, EngineActivity.class);
                engine.putExtra("nativeVulkanBackend", true);
                engine.putExtra("nativeVulkanHeight", height);
                engine.putExtra(EngineActivity.EXTRA_LAUNCHER_MAP, map);
                engine.putExtra(EngineActivity.EXTRA_TOUCH_OPACITY, opacity);
                engine.putExtra(EngineActivity.EXTRA_TOUCH_SCALE, scale);
                engine.putExtra(EngineActivity.EXTRA_GAME_ROOT, gamePath);
                engine.putExtra(EngineActivity.EXTRA_USER_ROOT, userPath);
                MAIN.post(() -> startActivity(engine));
                return "";
            } catch (IOException | RuntimeException | JSONException failure) {
                Log.e("BlackOpsARM", "Launcher start failed", failure);
                return failure.getMessage() == null ? "Could not start the game" : failure.getMessage();
            }
        }

        @JavascriptInterface public void openImport() {
            MAIN.post(() -> startActivity(new Intent(LauncherActivity.this, AssetImportActivity.class)));
        }

        @JavascriptInterface public void quit() { MAIN.post(LauncherActivity.this::finishAndRemoveTask); }
    }
}
