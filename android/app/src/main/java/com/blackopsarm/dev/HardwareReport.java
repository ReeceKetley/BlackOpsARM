package com.blackopsarm.dev;

import android.app.ActivityManager;
import android.content.Context;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.os.StatFs;
import android.view.Display;
import org.json.JSONArray;
import org.json.JSONObject;
import java.util.Locale;

/** Read-only device report. No device identifiers or permission requests. */
final class HardwareReport {
    private static String bytes(long value) {
        return String.format(Locale.ROOT, "%.2f GiB", value / 1073741824.0);
    }
    static String read(Context context, String[] graphics, int renderHeight) {
        JSONArray groups = new JSONArray();
        try {
            groups.put(group("Device", new String[][] {
                {"Manufacturer", Build.MANUFACTURER}, {"Model", Build.MODEL},
                {"Android", Build.VERSION.RELEASE + " · API " + Build.VERSION.SDK_INT},
                {"Security patch", Build.VERSION.SECURITY_PATCH}
            }));
            groups.put(group("Processor", new String[][] {
                {"SoC", Build.VERSION.SDK_INT >= 31 ? Build.SOC_MODEL : Build.HARDWARE},
                {"Platform", Build.HARDWARE},
                {"CPU cores available", Integer.toString(Runtime.getRuntime().availableProcessors())},
                {"Supported ABIs", String.join(", ", Build.SUPPORTED_ABIS)}
            }));
            ActivityManager.MemoryInfo memory = new ActivityManager.MemoryInfo();
            context.getSystemService(ActivityManager.class).getMemoryInfo(memory);
            StatFs disk = new StatFs(context.getFilesDir().getAbsolutePath());
            groups.put(group("Memory & storage", new String[][] {
                {"Total RAM", bytes(memory.totalMem)}, {"Available RAM", bytes(memory.availMem)},
                {"App storage volume", bytes(disk.getTotalBytes())}, {"Available storage", bytes(disk.getAvailableBytes())}
            }));
            groups.put(group("Graphics", new String[][] {
                {"GPU", graphics != null && graphics.length > 0 ? graphics[0] : ""},
                {"Vulkan supported", graphics != null && graphics.length > 1 ? graphics[1] : ""},
                {"Driver version (raw)", graphics != null && graphics.length > 2 ? graphics[2] : ""},
                {"Game render height", renderHeight + "p"}
            }));
            Display display = context.getSystemService(DisplayManager.class).getDisplay(Display.DEFAULT_DISPLAY);
            if (display != null) {
                Display.Mode mode = display.getMode();
                groups.put(group("Display", new String[][] {
                    {"Current display mode", mode.getPhysicalWidth() + " × " + mode.getPhysicalHeight()},
                    {"Refresh rate", String.format(Locale.ROOT, "%.2f Hz", mode.getRefreshRate())}
                }));
            }
            return new JSONObject().put("groups", groups).toString();
        } catch (Exception failure) {
            android.util.Log.e("BlackOpsARM", "Hardware report unavailable", failure);
            return "{\"error\":\"Hardware information is unavailable\"}";
        }
    }
    private static JSONObject group(String title, String[][] values) throws org.json.JSONException {
        JSONArray rows = new JSONArray();
        for (String[] row : values) rows.put(new JSONArray().put(row[0]).put(
                row[1] == null || row[1].trim().isEmpty() || "unknown".equalsIgnoreCase(row[1]) ? "Not reported" : row[1]));
        return new JSONObject().put("title", title).put("rows", rows);
    }
}
