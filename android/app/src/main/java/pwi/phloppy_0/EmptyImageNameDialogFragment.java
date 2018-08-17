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
import android.view.LayoutInflater;
import android.widget.EditText;

public class EmptyImageNameDialogFragment extends DialogFragment {
    public interface EmptyImageNameDialogListener {
        void onDialogPositiveClick(int id, String name);
        void onDialogNegativeClick(int id, String name);
    }

    private EmptyImageNameDialogListener listener;

    public static EmptyImageNameDialogFragment newInstance(boolean raw) {
        EmptyImageNameDialogFragment frag = new EmptyImageNameDialogFragment();
        Bundle args = new Bundle();
        args.putString("is-raw", Boolean.toString(raw));
        frag.setArguments(args);
        return frag;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        listener = (EmptyImageNameDialogListener) context;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        boolean raw = Boolean.valueOf(args.getString("is-raw"));
        final int clickId = raw ? R.integer.create_raw_image : R.integer.create_adf_image;
        final int layoutId = raw ? R.layout.dialog_empty_raw_image_name : R.layout.dialog_empty_adf_image_name;
        final int imageNameId = raw ? R.id.raw_image_name : R.id.adf_image_name;

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        LayoutInflater inflater = getActivity().getLayoutInflater();
        builder.setView(inflater.inflate(layoutId, null))
                .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        String name = ((EditText) EmptyImageNameDialogFragment.this.getDialog()
                                .findViewById(imageNameId)).getText().toString();
                        listener.onDialogPositiveClick(clickId, name);
                    }
                })
                .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        String name = ((EditText) EmptyImageNameDialogFragment.this.getDialog()
                                .findViewById(imageNameId)).getText().toString();
                        listener.onDialogNegativeClick(clickId, name);
                    }
                });
        return builder.create();
    }
}
