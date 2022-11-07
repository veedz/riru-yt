
if [ $BOOTMODE = false ]; then
	ui_print "- Installing through TWRP Not supported"
	ui_print "- Intsall this module via Magisk Manager"
	abort "- Aborting installation !!"
fi

PKGNAME=com.google.android.youtube
APPNAME="YouTube"

if [ ! -d "/data/data/$PKGNAME" ];
then
	ui_print "- $APPNAME app is not installed"
	ui_print "- Install $APPNAME from PlayStore"
	abort "- Aborting installation !!"
fi

STOCKAPPVER=$(dumpsys package $PKGNAME | grep versionName | cut -d= -f 2 | sed -n '1p')
RVAPPVER=$(grep version= "$MODPATH/module.prop" | sed 's/version=v//')

if [ "$STOCKAPPVER" != "$RVAPPVER" ]
then
	ui_print "- Installed $APPNAME version = $STOCKAPPVER"
	ui_print "- $APPNAME Revanced version = $RVAPPVER"
	ui_print "- App Version Mismatch !!"
	ui_print "- Get the module matching the version number."
	abort "- Aborting installation !!"
fi

api_level_arch_detect
[ ! -d "$MODPATH/libs/$ABI" ] && abort "! $ABI not supported"
cp -af "$MODPATH/libs/$ABI/youtubervx" "$MODPATH/youtubervx"
rm -rf "$MODPATH/libs"
chcon -R u:object_r:system_file:s0 "$MODPATH/youtubervx"
chmod -R 755 "$MODPATH/youtubervx"
