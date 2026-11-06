package com.epicgames.unreal;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.UriPermission;
import android.net.Uri;
import android.provider.DocumentsContract;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;

/** Writes completed SQLite snapshots to a player-selected, uninstall-safe folder. */
final class OGExternalBackupStore {
    private static final String DIRECTORY = "backup_directory";
    private static final long DAY_MS = 24L * 60 * 60 * 1000;
    private static final int DAILY_SNAPSHOTS = 7;
    private static final int WEEKLY_SNAPSHOTS = 4;
    private static final AtomicBoolean writing = new AtomicBoolean(false);
    private static final ExecutorService io = Executors.newSingleThreadExecutor();
    private static volatile String lastState = "";

    private static SharedPreferences settings(Context context) {
        return context.getSharedPreferences("NoWorldExternalBackups", Context.MODE_PRIVATE);
    }

    static void pickFolder(Activity activity, int requestCode) {
        activity.runOnUiThread(() -> {
            try {
                Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                picker.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
                activity.startActivityForResult(picker, requestCode);
            } catch (RuntimeException error) {
                lastState = "folder_unavailable";
            }
        });
    }

    static void acceptFolder(Activity activity, int resultCode, Intent data) {
        if (resultCode != Activity.RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        Uri directory = data.getData();
        int flags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if (flags != (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION)) {
            lastState = "folder_unavailable";
            return;
        }
        try {
            activity.getContentResolver().takePersistableUriPermission(directory, flags);
            settings(activity).edit().putString(DIRECTORY, directory.toString()).commit();
            lastState = "folder_ready";
        } catch (RuntimeException error) {
            lastState = "folder_unavailable";
        }
    }

    private static Uri writableDirectory(Context context) {
        String selected = settings(context).getString(DIRECTORY, "");
        if (selected.isEmpty()) {
            return null;
        }
        Uri directory = Uri.parse(selected);
        for (UriPermission permission : context.getContentResolver().getPersistedUriPermissions()) {
            if (permission.isReadPermission() && permission.isWritePermission()
                    && directory.equals(permission.getUri())) {
                return directory;
            }
        }
        return null;
    }

    static String state(Context context) {
        if (writing.get()) {
            return "writing";
        }
        if (writableDirectory(context) == null) {
            return lastState.equals("folder_unavailable") ? lastState : "no_folder";
        }
        return lastState.isEmpty() ? "folder_ready" : lastState;
    }

    static boolean shouldQueue(Context context) {
        Uri directory = writableDirectory(context);
        return directory != null && !writing.get()
                && settings(context).getLong("last_day:" + directory, -1) < System.currentTimeMillis() / DAY_MS;
    }

    static void queue(Context activity, String sourcePath, String name) {
        Context context = activity.getApplicationContext();
        File source = new File(sourcePath);
        Uri directory = writableDirectory(context);
        if (directory == null || !writing.compareAndSet(false, true)) {
            source.delete();
            return;
        }
        io.execute(() -> {
            ContentResolver resolver = context.getContentResolver();
            Uri staged = null;
            try {
                Uri root = DocumentsContract.buildDocumentUriUsingTree(
                        directory, DocumentsContract.getTreeDocumentId(directory));
                staged = DocumentsContract.createDocument(resolver, root, "application/octet-stream", name + ".partial");
                if (staged == null) {
                    throw new IllegalStateException("Could not create backup document.");
                }
                MessageDigest expected = MessageDigest.getInstance("SHA-256");
                try (InputStream in = new FileInputStream(source);
                     OutputStream out = resolver.openOutputStream(staged, "w")) {
                    if (out == null) {
                        throw new IllegalStateException("Could not open backup document.");
                    }
                    byte[] buffer = new byte[128 * 1024];
                    for (int count; (count = in.read(buffer)) != -1; ) {
                        out.write(buffer, 0, count);
                        expected.update(buffer, 0, count);
                    }
                    out.flush();
                }
                MessageDigest written = MessageDigest.getInstance("SHA-256");
                try (InputStream in = resolver.openInputStream(staged)) {
                    if (in == null) {
                        throw new IllegalStateException("Could not read backup document.");
                    }
                    byte[] buffer = new byte[128 * 1024];
                    for (int count; (count = in.read(buffer)) != -1; ) {
                        written.update(buffer, 0, count);
                    }
                }
                if (!Arrays.equals(expected.digest(), written.digest())) {
                    throw new IllegalStateException("Backup content was altered during transfer.");
                }
                Uri published = DocumentsContract.renameDocument(resolver, staged, name);
                if (published == null) {
                    throw new IllegalStateException("Could not publish completed backup document.");
                }
                staged = null;
                long now = System.currentTimeMillis();
                rememberAndRotate(context, resolver, directory, published, now);
                lastState = "backed_up";
            } catch (Exception error) {
                if (staged != null) {
                    deleteOwnedDocument(resolver, staged);
                }
                lastState = "backup_failed";
            } finally {
                source.delete(); // Only the dedicated internal staging snapshot owned by this transfer.
                writing.set(false);
            }
        });
    }

    private static void rememberAndRotate(Context context, ContentResolver resolver,
                                          Uri directory, Uri published, long now) throws Exception {
        SharedPreferences preferences = settings(context);
        String key = "owned:" + directory;
        JSONArray previous;
        try {
            previous = new JSONArray(preferences.getString(key, "[]"));
        } catch (Exception ignored) {
            previous = new JSONArray(); // Unknown entries are never deleted.
        }
        JSONArray kept = new JSONArray();
        kept.put(new JSONObject().put("uri", published.toString()).put("utc", now));
        Set<Long> olderWeeks = new HashSet<>();
        for (int index = 0; index < previous.length(); ++index) {
            JSONObject entry = previous.optJSONObject(index);
            if (entry == null || entry.optString("uri").isEmpty() || entry.optLong("utc") <= 0) {
                continue;
            }
            long week = entry.optLong("utc") / (7 * DAY_MS);
            boolean keep = index < DAILY_SNAPSHOTS - 1
                    || (!olderWeeks.contains(week) && olderWeeks.size() < WEEKLY_SNAPSHOTS && olderWeeks.add(week));
            if (keep || !deleteOwnedDocument(resolver, Uri.parse(entry.optString("uri")))) {
                kept.put(entry);
            }
        }
        preferences.edit().putString(key, kept.toString()).putLong("last_day:" + directory, now / DAY_MS).commit();
    }

    private static boolean deleteOwnedDocument(ContentResolver resolver, Uri document) {
        try {
            return DocumentsContract.deleteDocument(resolver, document);
        } catch (Exception ignored) {
            return false;
        }
    }
}
