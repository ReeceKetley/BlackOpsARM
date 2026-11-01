package com.blackopsarm.dev;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Color;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/** Shared offline shell for installation and the in-engine launcher. */
final class LauncherWebView {
    static final String URL = "file:///android_asset/launcher/index.html";
    private static boolean initialized;

    @SuppressLint("SetJavaScriptEnabled")
    static WebView create(Activity owner, Object bridge) {
        // Installation lives in the host process; the launcher lives in :engine.
        // Android forbids those two processes sharing a WebView data directory.
        // Keep the existing engine directory (and its saved launcher settings).
        if (!initialized) {
            if (owner instanceof AssetImportActivity) WebView.setDataDirectorySuffix("installer");
            initialized = true;
        }
        WebView view = new WebView(owner);
        view.setBackgroundColor(Color.rgb(6, 9, 8));
        view.getSettings().setJavaScriptEnabled(true);
        view.getSettings().setDomStorageEnabled(true);
        view.getSettings().setMediaPlaybackRequiresUserGesture(false);
        view.getSettings().setAllowContentAccess(false);
        view.getSettings().setAllowFileAccessFromFileURLs(true);
        view.getSettings().setBlockNetworkLoads(true);
        view.setWebViewClient(new WebViewClient() {
            @Override public boolean shouldOverrideUrlLoading(WebView web, WebResourceRequest request) {
                return !request.getUrl().toString().startsWith("file:///android_asset/launcher/");
            }
        });
        view.addJavascriptInterface(bridge, "BlackOpsARM");
        return view;
    }
}
