package com.blackopsarm.dev;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.system.ErrnoException;
import android.system.Os;
import android.system.StructStat;
import android.util.AtomicFile;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CancellationException;
import java.util.concurrent.atomic.AtomicBoolean;

/** SAF is used only for import. The native engine receives ordinary app-owned POSIX paths. */
final class AssetImportStore {
    private static final long RESERVE = 64L * 1024 * 1024;
    private static final int MAX_FILES = 10000;
    private final Context context;

    interface Progress { void update(String text, int done, int count); }

    AssetImportStore(Context context) { this.context = context.getApplicationContext(); }

    File gameRoot() throws IOException {
        File root = context.getExternalFilesDir(null);
        if (root == null || !Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(root))) {
            throw new IOException("Game storage is unavailable. Reconnect the storage and try again.");
        }
        return directory(AssetPaths.child(root, "game"));
    }

    File userRoot() throws IOException { return directory(AssetPaths.child(context.getFilesDir(), "user")); }

    private static File directory(File file) throws IOException {
        if ((!file.isDirectory() && !file.mkdirs()) || !file.canWrite()) {
            throw new IOException("Cannot use storage folder: " + file);
        }
        File canonical = file.getCanonicalFile();
        if (!canonical.getPath().equals(file.getAbsolutePath())) {
            throw new IOException("Storage folder must not be a symbolic link: " + file);
        }
        return canonical;
    }

    private AtomicFile receipt() {
        return new AtomicFile(new File(context.getFilesDir(), "asset-import.json"));
    }

    private JSONObject readReceipt() throws IOException {
        try {
            return new JSONObject(new String(receipt().readFully(), StandardCharsets.UTF_8));
        } catch (java.io.FileNotFoundException absent) {
            return new JSONObject();
        } catch (JSONException invalid) {
            throw new IOException("The saved import record is damaged. Import your files again.", invalid);
        }
    }

    private void writeReceipt(JSONObject record) throws IOException {
        AtomicFile file = receipt();
        FileOutputStream output = null;
        try {
            output = file.startWrite();
            output.write(record.toString().getBytes(StandardCharsets.UTF_8));
            output.getFD().sync();
            file.finishWrite(output);
            // AtomicFile logs some rename/sync failures instead of throwing; never acknowledge
            // a transaction until the record that can actually be reopened matches our result.
            if (!java.util.Arrays.equals(file.readFully(), record.toString().getBytes(StandardCharsets.UTF_8))) {
                throw new IOException("Cannot save the import record. Check available storage.");
            }
        } catch (IOException failure) {
            if (output != null) file.failWrite(output);
            throw failure;
        }
    }

    /** A process-owned exclusive lock also prevents importing while SDL is using the files. */
    final class UseLock implements AutoCloseable {
        private final RandomAccessFile file;
        private final FileLock lock;
        UseLock() throws IOException {
            file = new RandomAccessFile(new File(context.getFilesDir(), "asset-import.lock"), "rw");
            FileLock acquired;
            try { acquired = file.getChannel().tryLock(); }
            catch (OverlappingFileLockException busy) { acquired = null; }
            catch (IOException failure) { file.close(); throw failure; }
            if (acquired == null) {
                file.close();
                throw new IOException("Game files are in use. Close the game or wait for the import to finish.");
            }
            lock = acquired;
        }
        @Override public void close() throws IOException {
            try { lock.release(); } finally { file.close(); }
        }
    }

    UseLock acquire() throws IOException { return new UseLock(); }

    /** Receipt + current file facts; never an assertion that native initialization or rendering passes. */
    void requireCommitted(File root) throws IOException {
        JSONObject record = readReceipt();
        JSONObject files = record.optJSONObject("files");
        if (record.optInt("version") != 1 || !record.optBoolean("complete") || files == null
                || !root.getCanonicalPath().equals(record.optString("gameRoot"))) {
            throw new IOException("Choose your original game folder and complete an import first.");
        }
        // Detect external removal/replacement without rereading gigabytes every time the game is launched.
        java.util.Iterator<String> names = files.keys();
        Set<String> recorded = new HashSet<>();
        while (names.hasNext()) {
            String path = names.next();
            recorded.add(path);
            File file = AssetPaths.child(root, path);
            JSONObject entry = files.optJSONObject(path);
            if (entry == null || !file.isFile() || file.length() != entry.optLong("bytes", -1)
                    || file.lastModified() != entry.optLong("modified", -1)
                    || entry.optString("sha256").length() != 64) {
                throw new IOException("Imported file changed or is missing: " + path + ". Import again.");
            }
        }
        rejectUnselected(root, "", recorded, 0);
        for (String path : AssetPaths.required()) {
            File file = AssetPaths.child(root, path);
            if (!files.has(path) || !file.isFile() || file.length() <= 0 || !file.canRead()) {
                throw new IOException("Missing English startup/Kino asset: " + path);
            }
        }
    }

    private static final class Source {
        final Uri uri;
        final String path;
        final long size;
        Source(Uri uri, String path, long size) { this.uri = uri; this.path = path; this.size = size; }
    }

    void importTree(Uri tree, AtomicBoolean cancelled, Progress progress) throws IOException {
        try (UseLock ignored = acquire()) {
            File root = gameRoot();
            userRoot();
            rejectLocalOverlap(tree, root);
            progress.update("Reading the selected folder…", 0, 0);
            List<Source> sources = new ArrayList<>();
            Set<String> ids = new HashSet<>();
            Set<String> paths = new HashSet<>();
            String id = DocumentsContract.getTreeDocumentId(tree);
            scan(tree, id, "", 0, sources, ids, paths, cancelled);
            Set<String> found = new HashSet<>();
            for (Source source : sources) found.add(source.path);
            for (String required : AssetPaths.required()) {
                if (!found.contains(required)) {
                    throw new IOException("Select the game folder containing main and zone. Missing: " + required);
                }
            }
            // Do not mix an old asset tree with a different, incomplete source choice.
            // Managed originals are retained; the user can choose the complete source again.
            rejectUnselected(root, "", found, 0);
            // Start a new transaction before any game file is changed. A killed/cancelled worker
            // leaves complete=false even though individual atomically installed files can be reused.
            JSONObject record = new JSONObject();
            JSONObject files = new JSONObject();
            put(record, "version", 1);
            put(record, "gameRoot", root.getPath());
            put(record, "sourceTree", tree.toString());
            put(record, "complete", false);
            put(record, "files", files);
            writeReceipt(record);
            int done = 0;
            int copied = 0;
            for (Source source : sources) {
                checkCancel(cancelled);
                File target = AssetPaths.child(root, source.path);
                directory(target.getParentFile());
                String digest = null;
                boolean same = false;
                if (target.exists()) {
                    if (!target.isFile()) throw new IOException("A folder occupies a game file path: " + source.path);
                    progress.update("Checking existing file " + (done + 1) + "/" + sources.size()
                            + "\n" + source.path, done, sources.size());
                    // A timestamp alone is not proof. Compare actual source and destination SHA256;
                    // retain equal files even after an interrupted import or a different source choice.
                    if (source.size < 0 || target.length() == source.size) {
                        digest = hashSource(source, target, cancelled);
                        same = digest.equals(hashFile(target, cancelled));
                    }
                }
                if (!same) {
                    progress.update("Copying file " + (done + 1) + "/" + sources.size()
                            + "\n" + source.path, done, sources.size());
                    digest = copy(source, target, cancelled, progress, done, sources.size());
                    copied++;
                }
                JSONObject entry = new JSONObject();
                put(entry, "bytes", target.length());
                put(entry, "modified", target.lastModified());
                put(entry, "sha256", digest);
                put(entry, "source", source.uri.toString());
                put(files, source.path, entry);
                writeReceipt(record);
                progress.update("Imported " + (++done) + "/" + sources.size() + " files", done, sources.size());
            }
            checkCancel(cancelled);
            put(record, "complete", true);
            writeReceipt(record);
            requireCommitted(root);
            progress.update("Import complete. " + copied + " files copied; " + (done - copied)
                    + " identical files kept.\nYour source files were preserved.", done, sources.size());
        } catch (SecurityException denied) {
            throw new IOException("Access to the selected folder was denied. Choose the folder again.", denied);
        } catch (IllegalArgumentException invalid) {
            throw new IOException("This folder provider cannot supply the original game files.", invalid);
        }
    }

    private static void rejectUnselected(File root, String relative, Set<String> expected, int depth) throws IOException {
        if (depth > 16) throw new IOException("Game storage contains an unsupported folder tree.");
        File directory = relative.isEmpty() ? root : AssetPaths.child(root, relative);
        File[] children = directory.listFiles();
        if (children == null) throw new IOException("Cannot read game storage: " + relative);
        for (File child : children) {
            String name = child.getName();
            if (name.startsWith(".bo1-import-") && name.endsWith(".part")) continue;
            String path = relative.isEmpty() ? name : relative + "/" + name;
            boolean folder = child.isDirectory();
            if (!AssetPaths.selected(path, folder)) continue;
            File checked = AssetPaths.child(root, path);
            if (folder) rejectUnselected(root, path, expected, depth + 1);
            else if (!expected.contains(path) || !checked.isFile()) {
                throw new IOException("A previously copied asset is absent from this import: " + path
                        + ". Choose the complete original folder again.");
            }
        }
    }

    private void scan(Uri tree, String id, String relative, int depth, List<Source> out,
            Set<String> ids, Set<String> paths, AtomicBoolean cancelled) throws IOException {
        checkCancel(cancelled);
        if (depth > 16 || !ids.add(id)) throw new IOException("The selected folder contains a recursive or repeated folder.");
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, id);
        String[] columns = { DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE,
                DocumentsContract.Document.COLUMN_SIZE, DocumentsContract.Document.COLUMN_FLAGS };
        try (Cursor cursor = context.getContentResolver().query(children, columns, null, null, null)) {
            if (cursor == null) throw new IOException("Cannot read the selected folder.");
            while (cursor.moveToNext()) {
                checkCancel(cancelled);
                String childId = cursor.getString(0);
                String name = cursor.getString(1);
                AssetPaths.checkName(name);
                String path = relative.isEmpty() ? name : relative + "/" + name;
                boolean folder = DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(2));
                // Import the original asset trees only. Windows programs, player data and logs
                // are unrelated to the native game root and are not copied.
                boolean selected = AssetPaths.selected(path, folder);
                if (!selected) continue;
                if (!paths.add(path.toLowerCase(Locale.ROOT))) throw new IOException("Duplicate game path: " + path);
                if (folder) {
                    scan(tree, childId, path, depth + 1, out, ids, paths, cancelled);
                } else {
                    if ((cursor.getInt(4) & DocumentsContract.Document.FLAG_VIRTUAL_DOCUMENT) != 0) {
                        throw new IOException("The file is a virtual document, not original bytes: " + path);
                    }
                    if (!ids.add(childId)) throw new IOException("Repeated source file: " + path);
                    long size = cursor.isNull(3) ? -1 : cursor.getLong(3);
                    if (size == 0 || size < -1) throw new IOException("Empty or invalid game file: " + path);
                    out.add(new Source(DocumentsContract.buildDocumentUriUsingTree(tree, childId), path, size));
                    if (out.size() > MAX_FILES) throw new IOException("Too many files. Select the original game folder.");
                }
            }
        }
    }

    private static void rejectLocalOverlap(Uri tree, File target) throws IOException {
        if (!"com.android.externalstorage.documents".equals(tree.getAuthority())) return;
        String id = DocumentsContract.getTreeDocumentId(tree);
        if (!id.startsWith("primary:")) return;
        File source = new File(Environment.getExternalStorageDirectory(), id.substring(8)).getCanonicalFile();
        String a = source.getPath() + File.separator;
        String b = target.getCanonicalPath() + File.separator;
        if (a.startsWith(b) || b.startsWith(a)) {
            throw new IOException("Choose your original folder outside this app's game storage.");
        }
    }

    private ParcelFileDescriptor open(Source source, File target) throws IOException {
        ParcelFileDescriptor descriptor = context.getContentResolver().openFileDescriptor(source.uri, "r");
        if (descriptor == null) throw new IOException("Cannot open " + source.path);
        try {
            if (target.exists()) {
                StructStat input = Os.fstat(descriptor.getFileDescriptor());
                StructStat output = Os.stat(target.getPath());
                if (input.st_dev == output.st_dev && input.st_ino == output.st_ino) {
                    throw new IOException("The selected source is already this app's game file: " + source.path);
                }
            }
            return descriptor;
        } catch (ErrnoException failure) {
            descriptor.close();
            throw new IOException("Cannot inspect the source file: " + source.path, failure);
        } catch (IOException failure) {
            descriptor.close();
            throw failure;
        }
    }

    private String hashSource(Source source, File target, AtomicBoolean cancelled) throws IOException {
        try (FileInputStream input = new ParcelFileDescriptor.AutoCloseInputStream(open(source, target))) {
            return hashStream(input, source.size, cancelled);
        }
    }

    private static String hashFile(File file, AtomicBoolean cancelled) throws IOException {
        try (FileInputStream input = new FileInputStream(file)) {
            return hashStream(input, file.length(), cancelled);
        }
    }

    private static String hashStream(FileInputStream input, long expected, AtomicBoolean cancelled) throws IOException {
        MessageDigest digest = sha256();
        byte[] buffer = new byte[1024 * 1024];
        long bytes = 0;
        int count;
        while ((count = input.read(buffer)) != -1) {
            checkCancel(cancelled);
            bytes += count;
            digest.update(buffer, 0, count);
        }
        if (expected >= 0 && bytes != expected) throw new IOException("Source size changed during import. Try again.");
        if (bytes == 0) throw new IOException("A game file was empty.");
        return hex(digest.digest());
    }

    private String copy(Source source, File target, AtomicBoolean cancelled, Progress progress,
            int done, int count) throws IOException {
        // Only our own abandoned temporary files are removed; original/source files are never deleted.
        File[] abandoned = target.getParentFile().listFiles((dir, name) ->
                name.startsWith(".bo1-import-") && name.endsWith(".part"));
        if (abandoned == null) throw new IOException("Cannot read the game storage folder.");
        for (File part : abandoned) {
            if (!part.delete()) throw new IOException("Cannot remove an unfinished copy: " + part.getName());
        }
        long usable = target.getParentFile().getUsableSpace();
        if (usable <= RESERVE || (source.size >= 0 && source.size > usable - RESERVE)) {
            throw new IOException("Not enough storage for " + source.path + ". Free space and import again.");
        }
        File temporary = new File(target.getParentFile(), ".bo1-import-" + UUID.randomUUID() + ".part");
        try {
            MessageDigest digest = sha256();
            long bytes = 0;
            long lastUpdate = 0;
            try (FileInputStream input = new ParcelFileDescriptor.AutoCloseInputStream(open(source, target));
                    FileOutputStream output = new FileOutputStream(temporary)) {
                byte[] buffer = new byte[1024 * 1024];
                int n;
                while ((n = input.read(buffer)) != -1) {
                    checkCancel(cancelled);
                    if (target.getParentFile().getUsableSpace() <= RESERVE + n) {
                        throw new IOException("Storage is full. Free space and import again.");
                    }
                    output.write(buffer, 0, n);
                    digest.update(buffer, 0, n);
                    bytes += n;
                    long now = android.os.SystemClock.uptimeMillis();
                    if (now - lastUpdate >= 500) {
                        progress.update("Copying " + (done + 1) + "/" + count + "\n" + source.path
                                + "\n" + (bytes / (1024 * 1024)) + " MB", done, count);
                        lastUpdate = now;
                    }
                }
                output.flush();
                output.getFD().sync();
            }
            if (bytes <= 0 || (source.size >= 0 && bytes != source.size)) {
                throw new IOException("Source size changed or file was empty: " + source.path);
            }
            String sha = hex(digest.digest());
            // FileOutputStream writes each original byte unchanged; sync and exact length
            // establish a completed local copy. The digest records the actual input stream.
            if (temporary.length() != bytes) throw new IOException("Incomplete copied file: " + source.path);
            checkCancel(cancelled);
            try { Os.rename(temporary.getPath(), target.getPath()); }
            catch (ErrnoException failure) { throw new IOException("Cannot save game file: " + source.path, failure); }
            return sha;
        } finally {
            if (temporary.exists() && !temporary.delete()) {
                android.util.Log.w("BlackOpsARM", "Could not remove partial file " + temporary);
            }
        }
    }

    private static MessageDigest sha256() {
        try { return MessageDigest.getInstance("SHA-256"); }
        catch (NoSuchAlgorithmException impossible) { throw new IllegalStateException(impossible); }
    }
    private static String hex(byte[] bytes) {
        StringBuilder out = new StringBuilder(64);
        for (byte b : bytes) out.append(String.format(Locale.ROOT, "%02x", b & 255));
        return out.toString();
    }
    private static void checkCancel(AtomicBoolean cancelled) {
        if (cancelled.get() || Thread.currentThread().isInterrupted()) throw new CancellationException();
    }
    private static void put(JSONObject object, String key, Object value) throws IOException {
        try { object.put(key, value); }
        catch (JSONException failure) { throw new IOException("Cannot record the import result.", failure); }
    }
}
