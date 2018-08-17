/*
 * phloppy_0 - Commodore Amiga floppy drive emulator
 * Copyright (C) 2016-2018 Piotr Wiszowaty
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package pwi.phloppy_0;

import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.Switch;
import android.widget.TextView;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.LinkedList;

public class MainActivity extends AppCompatActivity
        implements Handler.Callback, Emulator.Callback, View.OnLongClickListener,
        SharedPreferences.OnSharedPreferenceChangeListener,
        EmptyImageNameDialogFragment.EmptyImageNameDialogListener,
        OverwriteDialogFragment.OverwriteDialogListener {
    private static final String TAG = "Phloppy_0/Main";

    private static final int FILE_CHOOSE_REQUEST_CODE = 65001;
    private static final int FILE_CHOOSE_REQUEST_CODE_NEW_0 = 65002;
    private static final int FILE_CHOOSE_REQUEST_CODE_NEW_1 = 65003;
    private static final int FILE_CHOOSE_REQUEST_CODE_NEW_2 = 65004;
    private static final int FILE_CHOOSE_REQUEST_CODE_NEW_3 = 65005;
    private static final String EXTRA_DRVNO = "pwi.phloppy_0.DRVNO";

    private static final String PREF_PATH0 = "path[0]";
    private static final String PREF_PATH1 = "path[1]";
    private static final String PREF_PATH2 = "path[2]";
    private static final String PREF_PATH3 = "path[3]";
    private static final String PREF_WRITE_PROTECT0 = "write_protect[0]";
    private static final String PREF_WRITE_PROTECT1 = "write_protect[1]";
    private static final String PREF_WRITE_PROTECT2 = "write_protect[2]";
    private static final String PREF_WRITE_PROTECT3 = "write_protect[3]";

    private static final int MSG_SET_TEXT = 0;
    private static final int MSG_SET_BUTTON0_TEXT = 1;
    private static final int MSG_SET_BUTTON1_TEXT = 2;
    private static final int MSG_SET_BUTTON2_TEXT = 3;
    private static final int MSG_SET_BUTTON3_TEXT = 4;

    private static final String DIRNAME = "Phloppy_0";

    private String host;
    private final int port = 4500;

    private Emulator emu;
    private boolean resume = false;
    private String[] path = {null, null, null, null};

    private Handler handler;
    private BroadcastReceiver receiver;

    private LinkedList<String> lines = new LinkedList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(TAG, "onCreate");

        PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        host = sharedPref.getString(SettingsActivity.KEY_PREF_HOST, "");

        File path = Environment.getExternalStoragePublicDirectory(DIRNAME);
        path.mkdirs();

        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        Button b = findViewById(R.id.button0);
        b.setOnLongClickListener(this);
        b.setText("DF0:");
        b = findViewById(R.id.button1);
        b.setOnLongClickListener(this);
        b.setText("DF1:");
        b = findViewById(R.id.button2);
        b.setOnLongClickListener(this);
        b.setText("DF2:");
        b = findViewById(R.id.button3);
        b.setOnLongClickListener(this);
        b.setText("DF3:");

        handler = new Handler(this);

        IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (networkActive()) {
                    startEmulator();
                } else {
                    stopEmulator();
                }
            }
        };
        registerReceiver(receiver, filter);

        sharedPref.registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        Log.d(TAG, "onDestroy");

        if (receiver != null) {
            unregisterReceiver(receiver);
        }

        stopEmulator();
    }

    @Override
    protected void onStart() {
        super.onStart();

        if (!networkActive()) {
            WifiManager wifi =
                    (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            wifi.setWifiEnabled(true);
        }
    }

    private void startEmulator() {
        Log.d(TAG, "Start emulator");
        if (emu == null) {
            emu = new Emulator(host, port, this);
            emu.start();

            SharedPreferences prefs = getPreferences(Context.MODE_PRIVATE);
            String s;
            if ((s = prefs.getString(PREF_PATH0, null)) != null) {
                if (!prefs.getBoolean(PREF_WRITE_PROTECT0, true)) {
                    ((Switch) findViewById(R.id.switch0)).setChecked(true);
                    emu.writeProtect(0, false);
                }
                insert(0, s);
            }
            if ((s = prefs.getString(PREF_PATH1, null)) != null) {
                if (!prefs.getBoolean(PREF_WRITE_PROTECT1, true)) {
                    ((Switch) findViewById(R.id.switch1)).setChecked(true);
                    emu.writeProtect(1, false);
                }
                insert(1, s);
            }
            if ((s = prefs.getString(PREF_PATH2, null)) != null) {
                if (!prefs.getBoolean(PREF_WRITE_PROTECT2, true)) {
                    ((Switch) findViewById(R.id.switch2)).setChecked(true);
                    emu.writeProtect(2, false);
                }
                insert(2, s);
            }
            if ((s = prefs.getString(PREF_PATH3, null)) != null) {
                if (!prefs.getBoolean(PREF_WRITE_PROTECT3, true)) {
                    ((Switch) findViewById(R.id.switch3)).setChecked(true);
                    emu.writeProtect(3, false);
                }
                insert(3, s);
            }
        }
    }

    private void stopEmulator() {
        Log.d(TAG, "Stop emulator");
        if (emu != null) {
            emu.close();
            emu = null;

            SharedPreferences prefs = getPreferences(Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putString(PREF_PATH0, path[0]);
            editor.putString(PREF_PATH1, path[1]);
            editor.putString(PREF_PATH2, path[2]);
            editor.putString(PREF_PATH3, path[3]);
            editor.putBoolean(PREF_WRITE_PROTECT0, !((Switch) findViewById(R.id.switch0)).isChecked());
            editor.putBoolean(PREF_WRITE_PROTECT1, !((Switch) findViewById(R.id.switch1)).isChecked());
            editor.putBoolean(PREF_WRITE_PROTECT2, !((Switch) findViewById(R.id.switch2)).isChecked());
            editor.putBoolean(PREF_WRITE_PROTECT3, !((Switch) findViewById(R.id.switch3)).isChecked());
            editor.commit();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            Intent i = new Intent(this, SettingsActivity.class);
            startActivity(i);
            return true;
        } else if (id == R.id.action_create_empty_adf_image) {
            if (emu != null) {
                DialogFragment dialog = EmptyImageNameDialogFragment.newInstance(false);
                dialog.show(getSupportFragmentManager(), "EmptyImageNameDialogFragment");
            }
        } else if (id == R.id.action_create_empty_raw_image) {
            if (emu != null) {
                DialogFragment dialog = EmptyImageNameDialogFragment.newInstance(true);
                dialog.show(getSupportFragmentManager(), "EmptyImageNameDialogFragment");
            }
        }
        return super.onOptionsItemSelected(item);
    }

    private void createEmptyADFImage(String dir, String name) {
        try {
            emu.createEmptyADFImage(dir, name);
            String msg = "Created " + new File(dir, name);
            showText(msg);
            Log.d(TAG, msg);
        } catch (Exception e) {
            showText("Error writing " + new File(dir, name) + ": " + e.getMessage());
            Log.e(TAG, e.getMessage());
        }
    }

    private void createEmptyRawImage(String dir, String name) {
        try {
            emu.createEmptyRawImage(dir, name);
            String msg = "Created " + new File(dir, name);
            showText(msg);
            Log.d(TAG, msg);
        } catch (Exception e) {
            showText("Error writing " + new File(dir, name) + ": " + e.getMessage());
            Log.e(TAG, e.getMessage());
        }
    }

    @Override
    public void onDialogPositiveClick(int id, String name) {
        if (emu != null) {
            String dir = Environment.getExternalStoragePublicDirectory(DIRNAME).getPath();
            if (id == R.integer.create_adf_image) {
                File f = new File(dir, name);
                if (f.exists()) {
                    DialogFragment dialog = OverwriteDialogFragment.newInstance(dir, name, false);
                    dialog.show(getSupportFragmentManager(), "OverwriteDialogFragment");
                } else {
                    createEmptyADFImage(dir, name);

                }
            } else if (id == R.integer.overwrite_adf_image) {
                createEmptyADFImage(dir, name);
            } else if (id == R.integer.create_raw_image) {
                File f = new File(dir, name);
                if (f.exists()) {
                    DialogFragment dialog = OverwriteDialogFragment.newInstance(dir, name, true);
                    dialog.show(getSupportFragmentManager(), "OverwriteDialogFragment");
                } else {
                    createEmptyRawImage(dir, name);
                }
            } else if (id == R.integer.overwrite_raw_image) {
                createEmptyRawImage(dir, name);
            }
        }
    }

    @Override
    public void onDialogNegativeClick(int id, String name) {
        Log.d(TAG, "Canceled creation of empty image: " + name);
        // save name in preferences
    }

    public void onButtonPress(View view) {
        if (view.getId() == R.id.button0) {
            selectFile(0);
        } else if (view.getId() == R.id.button1) {
            selectFile(1);
        } else if (view.getId() == R.id.button2) {
            selectFile(2);
        } else if (view.getId() == R.id.button3) {
            selectFile(3);
        }
    }

    private void selectFile(int drvno) {
        if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            Intent intent = new Intent("org.openintents.action.PICK_FILE");
            intent.putExtra(EXTRA_DRVNO, drvno);
            try {
                startActivityForResult(Intent.createChooser(intent, "Select file"), FILE_CHOOSE_REQUEST_CODE);
            } catch (ActivityNotFoundException e) {
            }
        } else {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            intent.putExtra(EXTRA_DRVNO, drvno);
            int requestCode =
                    drvno == 0 ? FILE_CHOOSE_REQUEST_CODE_NEW_0 :
                    drvno == 1 ? FILE_CHOOSE_REQUEST_CODE_NEW_1 :
                    drvno == 2 ? FILE_CHOOSE_REQUEST_CODE_NEW_2 :
                    FILE_CHOOSE_REQUEST_CODE_NEW_3;
            startActivityForResult(intent, requestCode);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        int drvno = -1;  /* note: custom extras are lost from ACTION_OPEN_DOCUMENT intent in Android 7 */
        switch (requestCode) {
            case FILE_CHOOSE_REQUEST_CODE:
                if (resultCode == RESULT_OK) {
                    Uri uri = data.getData();
                    if ("file".equalsIgnoreCase(uri.getScheme())) {
                        String path = uri.getPath();
                        drvno = data.getIntExtra(EXTRA_DRVNO, -1);
                        insert(drvno, path);
                        drvno = -1;     // Restore value so we don't choose file again later
                    } else {
                        // Ignore content other than files
                    }
                }
                break;
            case FILE_CHOOSE_REQUEST_CODE_NEW_0:
                drvno = 0;
                break;
            case FILE_CHOOSE_REQUEST_CODE_NEW_1:
                drvno = 1;
                break;
            case FILE_CHOOSE_REQUEST_CODE_NEW_2:
                drvno = 2;
                break;
            case FILE_CHOOSE_REQUEST_CODE_NEW_3:
                drvno = 3;
                break;
        }
        if (drvno >= 0) {
            if (resultCode == RESULT_OK) {
                Uri uri = data.getData();
                try {
                    String[] tokens = uri.getLastPathSegment().split(":");
                    String path = tokens[1].replace(DIRNAME, Environment.getExternalStoragePublicDirectory(DIRNAME).getPath());
                    insert(drvno, path);
                } catch (Exception e) {
                    Log.e(TAG, e.getMessage(), e);
                }
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void insert(int drvno, String path) {
        String message = "Insert drive " + drvno + ": " + path;
        showText(message);
        Log.d(TAG, message);

        this.path[drvno] = path;

        if (path != null) {
            File file = new File(path);
            if (drvno == 0) {
                ((Button) findViewById(R.id.button0)).setText("DF0:" + file.getName());
            } else if (drvno == 1) {
                ((Button) findViewById(R.id.button1)).setText("DF1:" + file.getName());
            } else if (drvno == 2) {
                ((Button) findViewById(R.id.button2)).setText("DF2:" + file.getName());
            } else if (drvno == 3) {
                ((Button) findViewById(R.id.button3)).setText("DF3:" + file.getName());
            }
            emu.insert(drvno, path);
        }
    }

    public boolean onLongClick(View view) {
        if (view.getId() == R.id.button0) {
            eject(0);
            return true;
        } else if (view.getId() == R.id.button1) {
            eject(1);
            return true;
        } else if (view.getId() == R.id.button2) {
            eject(2);
            return true;
        } else if (view.getId() == R.id.button3) {
            eject(3);
            return true;
        } else {
            return false;
        }
    }

    private void eject(int drvno) {
        showText("Eject drive " + drvno);

        this.path[drvno] = null;

        if (drvno == 0) {
            ((Button) findViewById(R.id.button0)).setText("DF0:");
        } else if (drvno == 1) {
            ((Button) findViewById(R.id.button1)).setText("DF1:");
        } else if (drvno == 2) {
            ((Button) findViewById(R.id.button2)).setText("DF2:");
        } else if (drvno == 3) {
            ((Button) findViewById(R.id.button3)).setText("DF3:");
        }

        emu.eject(drvno);
    }

    public void onSwitchChange(View view) {
        int drvno = -1;
        Switch sw = (Switch) view;
        if (sw.getId() == R.id.switch0) {
            drvno = 0;
        } else if (sw.getId() == R.id.switch1) {
            drvno = 1;
        } else if (sw.getId() == R.id.switch2) {
            drvno = 2;
        } else if (sw.getId() == R.id.switch3) {
            drvno = 3;
        }
        boolean flag = !sw.isChecked();
        showText("Write protect drive " + drvno + ": " + (flag ? "on" : "off"));
        if (emu != null) {
            emu.writeProtect(drvno, flag);
        }
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
        String host = prefs.getString(SettingsActivity.KEY_PREF_HOST, "");
        if (this.host == null && host != null || !this.host.equals(host)) {
            this.host = host;
            stopEmulator();
            startEmulator();
        }
    }

    @Override
    public boolean handleMessage(android.os.Message message) {
        if (message.what == MSG_SET_TEXT) {
            SimpleDateFormat fmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            lines.addFirst(fmt.format(new Date()) + "\n" + message.obj);
            if (lines.size() > 5) {
                lines.removeLast();
            }
            StringBuilder b = new StringBuilder();
            boolean first = true;
            for (String s : lines) {
                if (first) {
                    first = false;
                } else {
                    b.append("\n\n");
                }
                b.append(s);
            }
            TextView tv = findViewById(R.id.textView);
            tv.setText(b.toString());
        } else if (message.what == MSG_SET_BUTTON0_TEXT) {
            Button b = findViewById(R.id.button0);
            b.setText((CharSequence) message.obj);
        } else if (message.what == MSG_SET_BUTTON1_TEXT) {
            Button b = findViewById(R.id.button1);
            b.setText((CharSequence) message.obj);
        } else if (message.what == MSG_SET_BUTTON2_TEXT) {
            Button b = findViewById(R.id.button2);
            b.setText((CharSequence) message.obj);
        } else if (message.what == MSG_SET_BUTTON3_TEXT) {
            Button b = findViewById(R.id.button3);
            b.setText((CharSequence) message.obj);
        } 

        return true;
    }

    private void showText(String text) {
        handler.obtainMessage(MSG_SET_TEXT, text).sendToTarget();
    }

    private void setButtonText(int drvno, String text) {
        if (drvno == 0) {
            handler.obtainMessage(MSG_SET_BUTTON0_TEXT, text);
        } else if (drvno == 1) {
            handler.obtainMessage(MSG_SET_BUTTON1_TEXT, text);
        } else if (drvno == 2) {
            handler.obtainMessage(MSG_SET_BUTTON2_TEXT, text);
        } else if (drvno == 3) {
            handler.obtainMessage(MSG_SET_BUTTON3_TEXT, text);
        }
    }

    @Override
    public void onEmulatorConnected(boolean success, String errorMessage) {
        Log.d(TAG, "Emulator connected: " + success);
        if (success) {
            showText("Connected to " + host + ":" + port);
        } else {
            showText("Cannot connect to " + host + ":" + port + " : " + errorMessage);
            emu = null;
        }
    }

    @Override
    public void onImageLoaded(int drvno, boolean success, String errorMessage) {
        if (success) {
            showText("Loaded drive " + drvno);
        } else {
            path[drvno] = null;
            if (drvno == 0) {
                setButtonText(0, "DF0:");
            } else if (drvno == 1) {
                setButtonText(1, "DF1:");
            } else if (drvno == 2) {
                setButtonText(2, "DF2:");
            } else if (drvno == 3) {
                setButtonText(3, "DF3:");
            }
            showText("Error loading drive " + drvno + ": " + errorMessage);
        }
    }

    @Override
    public void onTrackWritten(int drvno, int tt) {
        showText("Wrote drive " + drvno + ", head " + (tt & 1) + ", cylinder " + (tt >> 1));
    }

    @Override
    public void onWriteError(int drvno, String errorMessage) {
        showText("Error writing drive " + drvno + ": " + errorMessage);
    }

    private boolean networkActive() {
        ConnectivityManager connectivityManager =
            (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        return networkInfo.isConnected();
    }
}
