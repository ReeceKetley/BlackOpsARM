package com.blackopsarm.dev;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;

/**
 * Entry point. The menu itself is the bundled HTML launcher shown inside {@link EngineActivity}
 * (one process); this activity only checks the install and routes to the engine or the importer.
 *
 * Extras: launchMap/launchHeight (restart for a map at another render size) and, in debug builds
 * only, autoMap/autoConfig/autoHeight (automated map tests: am start -n .../.LauncherActivity --es autoMap zombie_theater).
 */
public final class LauncherActivity extends Activity {
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    // The tested settings (540p, SGSR, 30 FPS, medium textures, no shadows) plus god mode.
    private static final String AUTO_CONFIG = String.join(System.lineSeparator(),
            "seta r_picmip_manual 1", "seta r_picmip 1", "seta r_picmip_bump 1", "seta r_picmip_spec 1",
            "seta r_lodScaleRigid 1", "seta r_lodScaleSkinned 1", "seta com_maxfps 30", "set r_nativeSgsr 1", "seta sm_enable 0",
            "set r_nativeBloom 0", "seta r_dof_enable 0", "seta fx_marks 0", "set r_nativeParallelRecord 2", "set bo1_testclient_god 1");

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        route(getIntent());
    }

    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        route(intent);
    }

    private void route(Intent request) {
        final WeakReference<LauncherActivity> receiver = new WeakReference<>(this);
        final Context application = getApplicationContext();
        final boolean debuggable = (getApplicationInfo().flags & android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE) != 0;
        final SharedPreferences prefs = getSharedPreferences("launcher", MODE_PRIVATE);
        String map = request.getStringExtra("launchMap");
        int height = request.getIntExtra("launchHeight", 0);
        String config = null;
        if (debuggable && request.getStringExtra("autoMap") != null) {
            map = request.getStringExtra("autoMap");
            height = request.getIntExtra("autoHeight", 540);
            config = request.getStringExtra("autoConfig");
            if (config == null) config = AUTO_CONFIG;
        }
        if (map != null && !LauncherBridge.knownMap(map)) { Log.e("BlackOpsARM", "Unknown launch map " + map); map = null; }
        if (height != 480 && height != 540 && height != 720) height = prefs.getInt("height", 540);
        if (height != 480 && height != 540 && height != 720) height = 540;
        final String launchMap = map;
        final int launchHeight = height;
        final String launchConfig = config;
        new Thread(() -> {
            String game = null, user = null;
            try {
                AssetImportStore store = new AssetImportStore(application);
                try (AssetImportStore.UseLock ignored = store.acquireWithin(4000)) {
                    File gameDir = store.gameRoot();
                    store.requireCommitted(gameDir);
                    if (launchConfig != null) {
                        File temporary = new File(store.userRoot(), "launcher.cfg.tmp");
                        try (FileOutputStream out = new FileOutputStream(temporary)) {
                            out.write(("// written by the launcher before each start\n" + launchConfig + "\n").getBytes(StandardCharsets.UTF_8));
                            out.getFD().sync();
                        }
                        if (!temporary.renameTo(new File(store.userRoot(), "launcher.cfg"))) throw new IOException("Could not save launcher settings");
                    }
                    game = gameDir.getPath();
                    user = store.userRoot().getPath();
                }
            } catch (IOException | RuntimeException notInstalled) {
                Log.i("BlackOpsARM", "Game files not ready: " + notInstalled.getMessage());
            }
            final String gamePath = game, userPath = user;
            MAIN.post(() -> {
                LauncherActivity activity = receiver.get();
                if (activity == null || activity.isDestroyed()) return;
                Intent next;
                if (gamePath == null || userPath == null) {
                    next = new Intent(activity, AssetImportActivity.class);
                } else {
                    next = new Intent(activity, EngineActivity.class)
                            .putExtra("nativeVulkanBackend", true)
                            .putExtra("nativeVulkanHeight", launchHeight)
                            .putExtra("nativeAutoKino", false)
                            .putExtra(EngineActivity.EXTRA_GAME_ROOT, gamePath)
                            .putExtra(EngineActivity.EXTRA_USER_ROOT, userPath);
                    if (launchMap != null) next.putExtra(EngineActivity.EXTRA_LAUNCHER_MAP, launchMap);
                }
                activity.startActivity(next);
                activity.finish();
            });
        }, "launcher-route").start();
    }
}
