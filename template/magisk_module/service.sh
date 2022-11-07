MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != 1 ]; do
    sleep 1
done

rm -rf "$MODDIR/base.apk"

PKGNAME=com.google.android.youtube

STOCKAPPVER=$(dumpsys package $PKGNAME | grep versionName | cut -d= -f 2 | sed -n '1p')
RVAPPVER=$(grep version= "$MODDIR/module.prop" | sed 's/version=v//')

if [ "$STOCKAPPVER" != "$RVAPPVER" ]
then
	exit
fi


YOUTUBE_APK="$(pm path com.google.android.youtube | head -1 | sed "s/^package://")"
if [ -z "$YOUTUBE_APK" ]; then
    exit
fi
ln -fs "$YOUTUBE_APK" "$MODDIR/base.apk"

magisk --clone-attr "$MODDIR/base.apk" "$MODDIR/revanced.apk"