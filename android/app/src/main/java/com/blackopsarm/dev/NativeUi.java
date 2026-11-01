package com.blackopsarm.dev;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Application;
import android.content.ClipboardManager;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** Actual Activity services; never initializes SDL or invents engine readiness. */
public final class NativeUi {
    private static final String TAG="BlackOpsARM-UI";
    private static final int DISPATCH_SECONDS=30;
    private static final Handler UI=new Handler(Looper.getMainLooper());
    private static WeakReference<Console> console=new WeakReference<>(null);
    private NativeUi() {}
    private static boolean alive(Activity activity) { return activity!=null && !activity.isFinishing() && !activity.isDestroyed(); }
    private interface Action<T> { T run(); }
    private static final class Dispatch<T> {
        final Action<T> action; final T failed;
        final CountDownLatch complete=new CountDownLatch(1);
        boolean cancelled; T result;
        Dispatch(Action<T> action,T failed) { this.action=action;this.failed=failed;result=failed; }
        void execute() { synchronized(this) { try { if(!cancelled)result=action.run(); } catch(RuntimeException error) { Log.e(TAG,"Activity action failed",error);result=failed; } finally { complete.countDown(); } } }
        T waitResult() {
            try {
                if(complete.await(DISPATCH_SECONDS,TimeUnit.SECONDS))return result;
            } catch(InterruptedException error) { Thread.currentThread().interrupt(); }
            synchronized(this) {
                // A started action completes while holding this monitor. Never
                // report timeout then execute a queued launch later.
                if(complete.getCount()==0)return result;
                cancelled=true;
            }
            Log.e(TAG,"UI action cancelled before execution");return failed;
        }
    }
    private static <T> T dispatch(Action<T> action,T failed) {
        if(Looper.myLooper()==Looper.getMainLooper()) { try{return action.run();}catch(RuntimeException error){Log.e(TAG,"Activity action failed",error);return failed;} }
        Dispatch<T> request=new Dispatch<>(action,failed);
        if(!UI.post(request::execute))return failed;
        return request.waitResult();
    }
    public static boolean retireStartupOverlay(Activity activity) {
        return dispatch(() -> alive(activity) && activity instanceof EngineActivity
                && ((EngineActivity)activity).retireStartupOverlayForNativeQuit(), false);
    }
    public static boolean launchUrl(Activity activity,String value) {
        return dispatch(() -> {
            if(!alive(activity) || value==null || value.isEmpty())return false;
            Uri uri=Uri.parse(value);
            if(uri.getScheme()==null || uri.getScheme().isEmpty())return false;
            activity.startActivity(new Intent(Intent.ACTION_VIEW,uri));
            return true; // Actual startActivity acknowledgement, not remote-page completion.
        },false);
    }
    public static byte[] clipboardText(Activity activity) {
        return dispatch(() -> {
            if(!alive(activity))return null;
            ClipboardManager clipboard=(ClipboardManager)activity.getSystemService(Context.CLIPBOARD_SERVICE);
            if(clipboard==null || !clipboard.hasPrimaryClip())return null;
            ClipData data=clipboard.getPrimaryClip();
            if(data==null || data.getItemCount()==0)return null;
            CharSequence value=data.getItemAt(0).coerceToText(activity);
            if(value==null)return null;
            if(value.length()>1024*1024){Log.e(TAG,"Clipboard text exceeds native capacity");return null;}
            byte[] bytes=value.toString().getBytes(StandardCharsets.UTF_8);
            if(bytes.length>1024*1024){Log.e(TAG,"Clipboard UTF-8 exceeds native capacity");return null;}
            return bytes;
        },null);
    }
    private abstract static class ActivityOwner implements Application.ActivityLifecycleCallbacks {
        final Activity activity; boolean registered,retired;
        ActivityOwner(Activity activity){this.activity=activity;}
        boolean observe(){if(!alive(activity))return false;activity.getApplication().registerActivityLifecycleCallbacks(this);registered=true;return true;}
        void retireObserver(){if(registered){activity.getApplication().unregisterActivityLifecycleCallbacks(this);registered=false;}}
        abstract void destroyed();
        public void onActivityDestroyed(Activity value){if(value==activity)destroyed();}
        public void onActivityCreated(Activity value,Bundle bundle){}
        public void onActivityStarted(Activity value){}
        public void onActivityResumed(Activity value){}
        public void onActivityPaused(Activity value){}
        public void onActivityStopped(Activity value){}
        public void onActivitySaveInstanceState(Activity value,Bundle bundle){}
    }
    private static final class Confirmation extends ActivityOwner {
        final String title,message; final CountDownLatch dismissed=new CountDownLatch(1);
        AlertDialog dialog; boolean yes;
        Confirmation(Activity activity,String title,String message){super(activity);this.title=title;this.message=message;}
        boolean show(){
            if(!observe())return false;
            try {
                dialog=new AlertDialog.Builder(activity).setTitle(title).setMessage(message)
                    .setPositiveButton("Yes",(value,which)->yes=true)
                    .setNegativeButton("No",(value,which)->yes=false).create();
                dialog.setOnDismissListener(value->finish());dialog.show();
                if(!dialog.isShowing()){finish();return false;}
                return true;
            }catch(RuntimeException error){Log.e(TAG,"Confirmation display failed",error);finish();return false;}
        }
        void finish(){
            if(retired)return;retired=true;
            try{retireObserver();}catch(RuntimeException error){Log.e(TAG,"Confirmation observer retirement failed",error);yes=false;}
            try{if(dialog!=null && dialog.isShowing())dialog.dismiss();}catch(RuntimeException error){Log.e(TAG,"Confirmation retirement failed",error);yes=false;}
            finally{dismissed.countDown();}
        }
        void destroyed(){yes=false;finish();}
    }
    public static boolean confirm(Activity activity,String title,String message){
        // Waiting on the UI looper would prevent showing/dismissing the dialog.
        if(Looper.myLooper()==Looper.getMainLooper()){Log.e(TAG,"Refusing confirmation wait on Android UI looper");return false;}
        if(!alive(activity) || title==null || message==null)return false;
        Confirmation request=new Confirmation(activity,title,message);
        if(!dispatch(request::show,false))return false;
        try { request.dismissed.await();return request.yes; }
        catch(InterruptedException error){Thread.currentThread().interrupt();UI.post(()->{request.yes=false;request.finish();});return false;}
    }
    private static native byte[] nativeReadConsoleOutput();
    private static native boolean nativeSubmitConsoleCommand(byte[] value);
    private static native boolean nativeConsoleCommandReady();
    private static final class Console extends ActivityOwner {
        AlertDialog dialog;TextView output,status;EditText input;Button send;ScrollView scroll;
        final Runnable refresh=()->refresh();
        Console(Activity activity){super(activity);}
        boolean show(){
            if(!observe())return false;
            try{
                LinearLayout contents=new LinearLayout(activity);contents.setOrientation(LinearLayout.VERTICAL);
                int pad=(int)(12*activity.getResources().getDisplayMetrics().density);contents.setPadding(pad,pad,pad,pad);
                scroll=new ScrollView(activity);output=new TextView(activity);output.setTextIsSelectable(true);scroll.addView(output);
                contents.addView(scroll,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,pad*22));
                status=new TextView(activity);contents.addView(status);
                input=new EditText(activity);input.setSingleLine(true);input.setHint("Engine command");contents.addView(input);
                dialog=new AlertDialog.Builder(activity).setTitle("BlackOpsARM console (last 128 KiB)").setView(contents)
                    .setPositiveButton("Send",null).setNegativeButton("Close",(value,which)->finish()).create();
                dialog.setOnDismissListener(value->finish());dialog.show();
                if(!dialog.isShowing()){finish();return false;}
                send=dialog.getButton(AlertDialog.BUTTON_POSITIVE);send.setOnClickListener(value->submit());
                refresh();return true;
            }catch(RuntimeException | LinkageError error){Log.e(TAG,"Console display failed",error);finish();return false;}
        }
        void refresh(){
            if(retired || !alive(activity)){finish();return;}
            try{
                byte[] bytes=nativeReadConsoleOutput();
                if(bytes==null){status.setText("Console output unavailable");finish();return;}
                output.setText(new String(bytes,StandardCharsets.UTF_8));
                boolean ready=nativeConsoleCommandReady();send.setEnabled(ready);input.setEnabled(ready);
                status.setText(ready?"Commands queue to the engine input owner":"Engine input owner unavailable");
                scroll.post(()->scroll.fullScroll(View.FOCUS_DOWN));
                UI.postDelayed(refresh,250);
            }catch(RuntimeException | LinkageError error){Log.e(TAG,"Console refresh failed",error);finish();}
        }
        void submit(){
            try{
                byte[] value=input.getText().toString().getBytes(StandardCharsets.UTF_8);
                if(nativeSubmitConsoleCommand(value)){input.setText("");status.setText("Command queued; execution requires engine polling");}
                else status.setText("Command refused: unavailable owner, full queue or invalid line");
            }catch(RuntimeException | LinkageError error){Log.e(TAG,"Console submission failed",error);status.setText("Command submission failed");}
        }
        void finish(){
            if(retired)return;retired=true;UI.removeCallbacks(refresh);
            try{retireObserver();}catch(RuntimeException error){Log.e(TAG,"Console observer retirement failed",error);}
            try{if(dialog!=null && dialog.isShowing())dialog.dismiss();}catch(RuntimeException error){Log.e(TAG,"Console retirement failed",error);}
            if(console.get()==this)console=new WeakReference<>(null);
        }
        void destroyed(){finish();}
    }
    public static boolean showConsole(Activity activity){
        return dispatch(()->{
            if(!alive(activity))return false;
            Console old=console.get();
            if(old!=null && old.activity==activity && !old.retired && old.dialog!=null && old.dialog.isShowing())return true;
            if(old!=null)old.finish();
            Console created=new Console(activity);if(!created.show())return false;
            console=new WeakReference<>(created);return true;
        },false);
    }
}
