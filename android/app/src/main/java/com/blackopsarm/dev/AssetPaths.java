package com.blackopsarm.dev;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** POSIX paths and the original English startup/Kino asset set, not an engine-readiness test. */
final class AssetPaths {
    private AssetPaths() {}

    static void checkName(String name) throws IOException {
        if (name == null || name.isEmpty() || name.equals(".") || name.equals("..")
                || name.startsWith(".bo1-import-") || name.endsWith(" ") || name.endsWith(".")) {
            throw new IOException("Unsupported file or folder name: " + name);
        }
        for (int i = 0; i < name.length(); i++) {
            char c = name.charAt(i);
            if (c < 32 || c == 127 || c == '/' || c == '\\' || c == ':' || c == 0) {
                throw new IOException("Unsupported file or folder name: " + name);
            }
        }
        if (name.getBytes(java.nio.charset.StandardCharsets.UTF_8).length > 240) {
            throw new IOException("File or folder name is too long: " + name);
        }
    }

    static File child(File root, String relative) throws IOException {
        if (relative == null || relative.isEmpty() || relative.startsWith("/")) {
            throw new IOException("Invalid asset path");
        }
        // The root is supplied by Context, whose standard Android ancestors may
        // have aliases (/data/user/0 and /data/data). Resolve that trusted root
        // first; aliases introduced below it by an owned child remain forbidden.
        File canonicalRoot = root.getCanonicalFile();
        File current = canonicalRoot;
        for (String name : relative.split("/", -1)) {
            checkName(name);
            current = new File(current, name);
        }
        File canonical = current.getCanonicalFile();
        String prefix = canonicalRoot.getPath() + File.separator;
        if (!canonical.getPath().startsWith(prefix)
                || !canonical.getPath().equals(current.getAbsolutePath())) {
            throw new IOException("Asset path leaves its storage folder: " + relative);
        }
        return canonical;
    }

    static List<String> required() {
        List<String> paths = new ArrayList<>();
        // These are the original retail IWD volumes, not generated replacements.
        for (int i = 0; i <= 41; i++) {
            paths.add(String.format(Locale.ROOT, "main/iw_%02d.iwd", i));
        }
        for (int i = 0; i <= 14; i++) {
            paths.add(String.format(Locale.ROOT, "main/localized_English_iw%02d.iwd", i));
        }
        // Actual Com_Init + common Zombies + Kino load/unload evidence, full428.
        String[] zones = { "code_pre_gfx_mp", "code_post_gfx_mp", "code_post_gfx",
                "patch_mp", "patch", "common_zombie", "zombie_theater" };
        for (String zone : zones) {
            paths.add("zone/common/" + zone + ".ff");
            paths.add("zone/English/en_" + zone + ".ff");
        }
        paths.add("zone/common/common_zombie_patch.ff");
        paths.add("zone/common/zombie_theater_patch.ff");
        return paths;
    }

    static boolean selected(String path, boolean folder) {
        if (folder) return path.equals("main") || path.equals("zone")
                || path.startsWith("main/") || path.startsWith("zone/");
        String lower = path.toLowerCase(Locale.ROOT);
        return path.equals("localization.txt") || (path.startsWith("main/") && lower.endsWith(".iwd"))
                || (path.startsWith("main/video/") && lower.endsWith(".bik"))
                || (path.startsWith("zone/") && lower.endsWith(".ff"));
    }
}
