# YouTube Revanced Dynamic Inject
Replace Youtube with YouTube ReVanced APK by using Riru and binding mount

## What is difference?
- More and more banking apps now detect the bind mount of YouTube Revanced as "rooted". Not like other revanced module, which globally bind mount apk and will be detected by banking apps. 
- To solve the problem, we use dynamic mount, bind mount `revanced.apk` everytime YouTube is launching. The bind mount only apply for Youtube so none of app should detect it.
- It also brings some benefits:
  - Avoid leaving youtube apk in `/data/app` when user uninstall or update YouTube because the `base.apk` that have bind mount cannot be removed.
  - Fix Youtube Revanced no longer working after soft reboot (aka zygote restart).
