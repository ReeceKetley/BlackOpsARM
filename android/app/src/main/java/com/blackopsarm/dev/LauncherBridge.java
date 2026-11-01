package com.blackopsarm.dev;

import android.util.Log;
import android.webkit.JavascriptInterface;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/** JavaScript bridge for the launcher menu that runs inside the engine activity. */
final class LauncherBridge {
    /** Engine-activity services the menu can use. Called on WebView binder threads. */
    interface Host {
        String hardwareInfo();
        String resumeStatus();
        File gameRoot();
        File userRoot();
        /** Height the running engine renders at (480, 540 or 720). */
        int runningHeight();
        /** True once a match has run: its renderer/database state is not reusable for another map. */
        boolean needsFreshEngine();
        /** Queue a map for the running engine. */
        boolean requestMap(String map);
        boolean requestResume();
        void applyTouch(float opacity, float scale, float look);
        void rememberHeight(int height);
        /** The engine's render size is fixed per process: start a new engine for this map. */
        void restartWithMap(String map, int height, boolean resume);
        void openImporter();
        void quit();
    }

    // Zone stems the menu can start; any other value is rejected.
    static final String[] MAPS = {"zombie_theater", "zombie_pentagon", "zombie_cosmodrome", "zombie_coast",
            "zombie_temple", "zombie_moon", "zombie_cod5_prototype", "zombie_cod5_asylum", "zombie_cod5_sumpf",
            "zombie_cod5_factory"};

    private final Host host;

    LauncherBridge(Host host) { this.host = host; }

    static boolean knownMap(String map) {
        for (String known : MAPS) if (known.equals(map)) return true;
        return false;
    }

    /** {"installed":bool,"maps":[stems whose zone files are present],"height":running render height} */
    @JavascriptInterface public String status() {
        JSONObject result = new JSONObject();
        try {
            JSONArray maps = new JSONArray();
            File game = host.gameRoot();
            for (String map : MAPS) if (new File(game, "zone/common/" + map + ".ff").isFile()) maps.put(map);
            result.put("version", BuildConfig.VERSION_NAME);
            result.put("installed", true); // the engine only runs on a committed install
            result.put("maps", maps);
            result.put("height", host.runningHeight());
            result.put("problem", JSONObject.NULL);
        } catch (JSONException | RuntimeException failure) {
            Log.e("BlackOpsARM", "Launcher status failed", failure);
        }
        return result.toString();
    }

    /** {"map":stem,"height":480|540|720,"config":"console lines","touchOpacity":0.2..1,"touchScale":0.7..1.4}; returns "" or an error. */
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
            if (!new File(host.gameRoot(), "zone/common/" + map + ".ff").isFile()) return "This map's files are not installed";
            File user = host.userRoot();
            File temporary = new File(user, "launcher.cfg.tmp");
            try (FileOutputStream out = new FileOutputStream(temporary)) {
                out.write(("// written by the launcher before each start\n" + config + "\n").getBytes(StandardCharsets.UTF_8));
                out.getFD().sync();
            }
            if (!temporary.renameTo(new File(user, "launcher.cfg"))) throw new IOException("Could not save launcher settings");
            float look = (float) Math.max(0.2, Math.min(2.0, request.optDouble("touchLook", 1.0)));
            host.applyTouch(opacity, scale, look);
            host.rememberHeight(height);
            boolean resume = request.optBoolean("resume", false);
            if (height != host.runningHeight() || host.needsFreshEngine()) { host.restartWithMap(map, height, resume); return ""; }
            if (resume) return host.requestResume() ? "" : "The saved session is not ready to resume";
            return host.requestMap(map) ? "" : "Could not start that map";
        } catch (IOException | RuntimeException | JSONException failure) {
            Log.e("BlackOpsARM", "Launcher start failed", failure);
            return failure.getMessage() == null ? "Could not start the game" : failure.getMessage();
        }
    }

    @JavascriptInterface public void openImport() { host.openImporter(); }
    @JavascriptInterface public String hardwareInfo() { return host.hardwareInfo(); }
    @JavascriptInterface public String resumeStatus() { return host.resumeStatus(); }

    @JavascriptInterface public void quit() { host.quit(); }
}
