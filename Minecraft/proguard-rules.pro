-keep class com.omni.craft.Engine {
    native <methods>;
    public *;
}

-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}

-keep class com.omni.craft.Activity { *; }
-keep class com.omni.craft.GameSurface { *; }

-keepattributes *Annotation*,Signature,InnerClasses,EnclosingMethod,Exceptions,SourceFile,LineNumberTable

-dontwarn kotlin.**
-dontwarn kotlinx.**
-dontwarn javax.annotation.**

-keep class kotlin.Metadata { *; }
-keep class kotlin.coroutines.** { *; }
-keep class kotlinx.coroutines.** { *; }

-optimizationpasses 5
-allowaccessmodification
-repackageclasses "o"
-overloadaggressively

-assumenosideeffects class android.util.Log {
    public static int v(...);
    public static int d(...);
}
