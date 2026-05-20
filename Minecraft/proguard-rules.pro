-keep class com.omni.craft.Engine {
    native <methods>;
    public static native *;
    public static *** native*(...);
}

-keepclassmembers class com.omni.craft.Engine {
    native <methods>;
}

-keep class com.omni.craft.Activity { *; }
-keep class com.omni.craft.Render   { *; }

-keepattributes *Annotation*
-keepattributes SourceFile,LineNumberTable
-keepattributes Signature
-keepattributes Exceptions

-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}

-dontwarn kotlin.**
-dontwarn kotlinx.**
-dontwarn javax.annotation.**

-keep class kotlin.Metadata { *; }
-keep class kotlin.reflect.** { *; }
-keepclassmembers class kotlin.Metadata { *; }

-keep class kotlinx.coroutines.** { *; }
-keepclassmembernames class kotlinx.** { volatile <fields>; }

-keep @interface kotlin.Metadata
-keepclassmembers class * {
    @kotlin.Metadata *;
}

-optimizationpasses 5
-allowaccessmodification
-repackageclasses "o"
-overloadaggressively
