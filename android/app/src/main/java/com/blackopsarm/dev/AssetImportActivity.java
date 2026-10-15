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
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.concurrent.CancellationException;
import java.util.concurrent.atomic.AtomicBoolean;

/** User-owned originals are selected through Android's folder picker; no bundled retail data. */
public final class AssetImportActivity extends Activity {
    private static final int CHOOSE_FOLDER = 30;
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    // The job owns only application context and a weak UI reference, so rotations keep the copy running.
    private static ImportJob job;
    private TextView status;
    private ProgressBar progress;
    private Button choose;
    private Button launch;
    private Button cancel;
    private boolean ready;
    private int refreshGeneration;

    @Override protected void onCreate(Bundle savedState) {
        super.onCreate(savedState);
        int padding = (int) (24 * getResources().getDisplayMetrics().density);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(padding, padding, padding, padding);
        TextView title = new TextView(this);
        title.setText("Black Ops Zombies");
        title.setTextSize(28);
        layout.addView(title);
        TextView explanation = new TextView(this);
        explanation.setText("Choose your original English game folder containing main and zone. "
                + "The app copies game assets into its own storage and keeps your originals. "
                + "Allow space for the copy (a full installation is about 12 GB). "
                + "Uninstalling the app removes its copy and saved settings.\n\n"
                + "Native Android development build. Gameplay and rendering are still being validated.");
        explanation.setPadding(0, padding, 0, padding);
        layout.addView(explanation);
        status = new TextView(this);
        status.setText("Checking imported files…");
        status.setTextIsSelectable(true);
        layout.addView(status);
        progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        layout.addView(progress);
        choose = button(layout, "Choose game folder", view -> selectFolder());
        launch = button(layout, "Launch game", view -> launchGame());
        cancel = button(layout, "Cancel import", view -> {
            if (job != null && job.running) {
                job.cancelled.set(true);
                status.setText("Stopping the import… Completed files can be reused when you import again.");
                cancel.setEnabled(false);
            }
        });
        ScrollView scroll = new ScrollView(this);
        scroll.addView(layout);
        setContentView(scroll);
        setButtons(false);
    }

    private Button button(LinearLayout parent, String text, View.OnClickListener action) {
        Button button = new Button(this);
        button.setText(text);
        button.setOnClickListener(action);
        parent.addView(button);
        return button;
    }

    @Override protected void onResume() {
        super.onResume();
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
        super.onDestroy();
    }

    private void setButtons(boolean busy) {
        choose.setEnabled(!busy);
        launch.setEnabled(!busy && ready);
        cancel.setEnabled(busy && job != null && !job.cancelled.get());
        cancel.setVisibility(busy ? View.VISIBLE : View.GONE);
        progress.setVisibility(busy ? View.VISIBLE : View.GONE);
    }

    private void selectFolder() {
        Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        picker.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        String previous = getPreferences(MODE_PRIVATE).getString("sourceTree", null);
        if (previous != null) picker.putExtra(android.provider.DocumentsContract.EXTRA_INITIAL_URI, Uri.parse(previous));
        try { startActivityForResult(picker, CHOOSE_FOLDER); }
        catch (ActivityNotFoundException unavailable) {
            status.setText("No folder picker is available on this device.");
        }
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != CHOOSE_FOLDER || result != RESULT_OK || data == null || data.getData() == null) return;
        Uri tree = data.getData();
        int permission = data.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION;
        if (permission == 0) {
            status.setText("The folder picker did not grant read access. Choose the folder again.");
            return;
        }
        try {
            getContentResolver().takePersistableUriPermission(tree, permission);
        } catch (SecurityException denied) {
            status.setText("This folder does not allow lasting access. Choose a local folder that Android can read.");
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
        ready = false;
        job = new ImportJob(getApplicationContext(), tree, this);
        display(job);
        new Thread(job, "game-asset-import").start();
    }

    private void display(ImportJob current) {
        status.setText(current.message);
        progress.setIndeterminate(current.count == 0);
        if (current.count > 0) { progress.setMax(current.count); progress.setProgress(current.done); }
        setButtons(current.running);
    }

    private void refresh(boolean launchAfter) {
        final int generation = ++refreshGeneration;
        final WeakReference<AssetImportActivity> receiver = new WeakReference<>(this);
        final Context application = getApplicationContext();
        choose.setEnabled(false);
        launch.setEnabled(false);
        new Thread(() -> {
            String failure = null;
            String game = null;
            String user = null;
            try {
                AssetImportStore store = new AssetImportStore(application);
                try (AssetImportStore.UseLock ignored = store.acquire()) {
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
                activity.setButtons(false);
                if (!activity.ready) {
                    activity.status.setText(problem == null ? "Imported files are unavailable." : problem);
                    if (job != null && !job.running && !job.success) activity.status.setText(job.message + "\n\n" + activity.status.getText());
                } else {
                    activity.status.setText(job != null && job.success ? job.message
                            : "Imported English startup and Kino files are available.\n"
                            + "Game storage: " + gamePath);
                    if (launchAfter) {
                        Intent engine = new Intent(activity, EngineActivity.class);
                        engine.putExtra("nativeVulkanPresentation",
                                activity.getIntent().getBooleanExtra("nativeVulkanPresentation", false));
                        engine.putExtra("nativeVulkanHeight", activity.getIntent().getIntExtra("nativeVulkanHeight", 0));
                        engine.putExtra("nativeVulkanBackend",
                                activity.getIntent().getBooleanExtra("nativeVulkanBackend", false));
                        engine.putExtra(EngineActivity.EXTRA_GAME_ROOT, gamePath);
                        engine.putExtra(EngineActivity.EXTRA_USER_ROOT, userPath);
                        activity.startActivity(engine);
                    }
                }
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
