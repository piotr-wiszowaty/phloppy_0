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

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AlertDialog;

import java.io.File;

public class OverwriteDialogFragment extends DialogFragment {
    public interface OverwriteDialogListener {
        void onDialogPositiveClick(int id, String name);
        void onDialogNegativeClick(int id, String name);
    }

    private OverwriteDialogListener listener;

    public static OverwriteDialogFragment newInstance(String dir, String name, boolean raw) {
        OverwriteDialogFragment frag = new OverwriteDialogFragment();
        Bundle args = new Bundle();
        args.putString("name", name);
        args.putString("dir", dir);
        args.putString("is-raw", Boolean.toString(raw));
        frag.setArguments(args);
        return frag;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        listener = (OverwriteDialogListener) context;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        boolean raw = Boolean.valueOf(args.getString("is-raw"));
        final int clickId = raw ? R.integer.overwrite_raw_image : R.integer.overwrite_adf_image;
        final int layoutId = raw ? R.layout.dialog_empty_raw_image_name : R.layout.dialog_empty_adf_image_name;
        final int imageNameId = raw ? R.id.raw_image_name : R.id.adf_image_name;

        final String dir = args.getString("dir");
        final String name = args.getString("name");
        final File file = new File(dir, name);
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(String.format(getString(R.string.dialog_overwrite_file), file.toString()))
                .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        listener.onDialogPositiveClick(clickId, name);
                    }
                })
                .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        listener.onDialogNegativeClick(clickId, name);
                    }
                });
        return builder.create();
    }
}
