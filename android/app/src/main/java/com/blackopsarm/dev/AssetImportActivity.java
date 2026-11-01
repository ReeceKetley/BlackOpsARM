package com.blackopsarm.dev;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.webkit.WebView;
import android.webkit.JavascriptInterface;
import org.json.JSONObject;
import org.json.JSONArray;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.concurrent.CancellationException;
import java.util.concurrent.atomic.AtomicBoolean;

/** User-owned originals are selected through Android's folder picker; no bundled retail data. */
public final class AssetImportActivity extends Activity {
    private static final int CHOOSE_FOLDER = 30;
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    // The job owns only application context and a weak UI reference, so rotations keep the copy running.
    private static volatile ImportJob job;
    private WebView menu;
    private volatile String message = "Checking game files…";
    private volatile boolean checking = true;
    private volatile boolean ready;
    private volatile String[] installedMaps = new String[0];
    private int refreshGeneration;

    @Override protected void onCreate(Bundle savedState) {
        super.onCreate(savedState);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        menu = LauncherWebView.create(this, new ImportBridge());
        setContentView(menu);
        menu.loadUrl(LauncherWebView.URL + "#import");
    }

    public final class ImportBridge {
        @JavascriptInterface public String status() {
            try {
                ImportJob current = job;
                JSONObject state = new JSONObject().put("busy", current != null && current.running)
                        .put("checking", checking).put("message", message)
                        .put("done", current == null ? 0 : current.done)
                        .put("count", current == null ? 0 : current.count);
                return new JSONObject().put("importer", true).put("installed", ready)
                        .put("version", BuildConfig.VERSION_NAME).put("maps", new JSONArray(installedMaps))
                        .put("import", state).toString();
            } catch (org.json.JSONException error) { throw new IllegalStateException(error); }
        }
        @JavascriptInterface public String resumeStatus() { return "{\"available\":false}"; }
        @JavascriptInterface public String hardwareInfo() { return HardwareReport.read(AssetImportActivity.this, null, 0); }
        @JavascriptInterface public void openImport() { runOnUiThread(() -> { if (!checking && (job == null || !job.running)) selectFolder(); }); }
        @JavascriptInterface public void cancelImport() { runOnUiThread(() -> {
            if (job != null && job.running) { job.cancelled.set(true); message = "Stopping… Completed files can be reused next time."; publish(); }
        }); }
        @JavascriptInterface public void finishImport() { runOnUiThread(() -> { if (ready && !checking && (job == null || !job.running)) launchGame(); }); }
        @JavascriptInterface public void quit() { runOnUiThread(() -> onBackPressed()); }
    }

    private void publish() {
        if (menu != null) menu.evaluateJavascript("window.onNativeImportChanged&&window.onNativeImportChanged()", null);
    }

    @Override public void onBackPressed() {
        if (job != null && job.running) {
            new android.app.AlertDialog.Builder(this).setTitle("Stop importing?")
                    .setMessage("Completed files can be reused. Your original files stay unchanged.")
                    .setPositiveButton("Stop", (dialog, which) -> { job.cancelled.set(true); finish(); })
                    .setNegativeButton("Keep importing", null).show();
        } else if (ready) { launchGame(); } else { finish(); }
    }

    @Override protected void onResume() {
        super.onResume();
        if (menu != null) menu.onResume();
        if (job != null) {
            job.activity = new WeakReference<>(this);
            if (job.running) { display(job); return; }
        }
        refresh(false);
    }

    @Override protected void onDestroy() {
        refreshGeneration++;
        if (job != null && job.activity.get() == this) {
            job.activity.clear();
            if (isFinishing() && job.running) job.cancelled.set(true);
        }
        if (menu != null) {
            menu.removeJavascriptInterface("BlackOpsARM");
            if (menu.getParent() instanceof android.view.ViewGroup)
                ((android.view.ViewGroup)menu.getParent()).removeView(menu);
            menu.destroy(); menu = null;
        }
        super.onDestroy();
    }

    @Override protected void onPause() {
        if (menu != null) menu.onPause();
        super.onPause();
    }

    private void selectFolder() {
        Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        picker.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        String previous = getPreferences(MODE_PRIVATE).getString("sourceTree", null);
        if (previous != null) picker.putExtra(android.provider.DocumentsContract.EXTRA_INITIAL_URI, Uri.parse(previous));
        try { startActivityForResult(picker, CHOOSE_FOLDER); }
        catch (ActivityNotFoundException unavailable) {
            setMessage("No folder picker is available on this device.");
        }
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != CHOOSE_FOLDER || result != RESULT_OK || data == null || data.getData() == null) return;
        Uri tree = data.getData();
        int permission = data.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION;
        if (permission == 0) {
            setMessage("The folder picker did not grant read access. Choose the folder again.");
            return;
        }
        try {
            getContentResolver().takePersistableUriPermission(tree, permission);
        } catch (SecurityException denied) {
            setMessage("This folder does not allow lasting access. Choose a local folder that Android can read.");
            return;
        }
        String previous = getPreferences(MODE_PRIVATE).getString("sourceTree", null);
        if (previous != null && !previous.equals(tree.toString())) {
            try { getContentResolver().releasePersistableUriPermission(Uri.parse(previous), Intent.FLAG_GRANT_READ_URI_PERMISSION); }
            catch (SecurityException alreadyRevoked) { /* The new folder already has its genuine grant. */ }
        }
        getPreferences(MODE_PRIVATE).edit().putString("sourceTree", tree.toString()).apply();
        if (job != null && job.running) return;
        refreshGeneration++;
        checking = false; ready = false;
        job = new ImportJob(getApplicationContext(), tree, this);
        display(job);
        new Thread(job, "game-asset-import").start();
    }

    private void setMessage(String text) { message = text; publish(); }

    private void display(ImportJob current) { message = current.message; checking = false; publish(); }

    private void refresh(boolean launchAfter) {
        final int generation = ++refreshGeneration;
        final WeakReference<AssetImportActivity> receiver = new WeakReference<>(this);
        final Context application = getApplicationContext();
        checking = true; publish();
        new Thread(() -> {
            String failure = null;
            String game = null;
            String user = null;
            try {
                AssetImportStore store = new AssetImportStore(application);
                try (AssetImportStore.UseLock ignored = store.acquireWithin(4000)) {
                    java.io.File gameDir = store.gameRoot();
                    store.requireCommitted(gameDir);
                    game = gameDir.getPath();
                    user = store.userRoot().getPath();
                }
            } catch (IOException | RuntimeException error) { failure = error.getMessage(); }
            final String problem = failure;
            final String gamePath = game;
            final String userPath = user;
            MAIN.post(() -> {
                AssetImportActivity activity = receiver.get();
                if (activity == null || activity.isDestroyed() || generation != activity.refreshGeneration) return;
                activity.ready = problem == null && gamePath != null && userPath != null;
                activity.checking = false;
                if (!activity.ready) {
                    activity.setMessage(problem == null ? "Imported files are unavailable." : problem);
                    if (job != null && !job.running && !job.success) activity.setMessage(job.message + "\n\n" + activity.message);
                } else {
                    activity.setMessage(job != null && job.success ? job.message
                            : "Game files are installed. Continue to the launcher or choose a folder to repair them.");
                    java.util.ArrayList<String> maps = new java.util.ArrayList<>();
                    for (String map : LauncherBridge.MAPS)
                        if (new java.io.File(gamePath, "zone/common/" + map + ".ff").isFile()) maps.add(map);
                    activity.installedMaps = maps.toArray(new String[0]);
                    if (launchAfter) {
                        activity.startActivity(new Intent(activity, LauncherActivity.class));
                        activity.finish(); // Never leave the installer underneath the engine.
                    }
                }
                activity.publish();
            });
        }, "game-storage-check").start();
    }

    private void launchGame() { refresh(true); }

    private static final class ImportJob implements Runnable {
        final Context context;
        final Uri tree;
        final AtomicBoolean cancelled = new AtomicBoolean();
        volatile WeakReference<AssetImportActivity> activity;
        volatile boolean running = true;
        volatile boolean success;
        volatile String message = "Preparing import…";
        volatile int done;
        volatile int count;
        ImportJob(Context context, Uri tree, AssetImportActivity activity) {
            this.context = context; this.tree = tree; this.activity = new WeakReference<>(activity);
        }
        void update(String text, int completed, int total) {
            message = text; done = completed; count = total;
            MAIN.post(() -> {
                AssetImportActivity target = activity.get();
                if (target != null && !target.isDestroyed()) target.display(this);
            });
        }
        @Override public void run() {
            try {
                new AssetImportStore(context).importTree(tree, cancelled, this::update);
                success = true;
            } catch (CancellationException stopped) {
                message = "Import cancelled. Completed files can be reused when you import again.";
            } catch (IOException | RuntimeException failure) {
                android.util.Log.e("BlackOpsARM", "Asset import failed", failure);
                message = "Import could not finish. " + (failure.getMessage() == null
                        ? "Choose your original folder and try again." : failure.getMessage());
            } finally {
                running = false;
                MAIN.post(() -> {
                    AssetImportActivity target = activity.get();
                    if (target != null && !target.isDestroyed()) { target.display(this); target.refresh(false); }
                });
            }
        }
    }
}
