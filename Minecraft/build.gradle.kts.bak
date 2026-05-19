plugins {
    id("com.android.application")
}

android {
    namespace = "com.omni.craft"
    compileSdk = 36
    ndkVersion = "29.0.14206865"

    defaultConfig {
        applicationId = "com.omni.craft"
        minSdk = 30
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                cppFlags("-std=c++26 -O3")
            }
        }
    }

    sourceSets {
        getByName("main") {
            // Backrooms yapısındaki gibi hem java hem kotlin dizinlerini set ediyoruz
            java.setSrcDirs(listOf("Source/Main/Kotlin"))
            kotlin.setSrcDirs(listOf("Source/Main/Kotlin"))
            res.setSrcDirs(listOf("Source/Main/Res"))
            manifest.srcFile("Source/Main/AndroidManifests.xml")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("Source/Main/Native/CMakeLists.txt")
            version = "4.3.2"
        }
    }

    // Backrooms stili tertemiz Java 25 tanımlaması
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_25
        targetCompatibility = JavaVersion.VERSION_25
    }

    // AGP 9.2+ içindeki yerleşik Kotlin Toolchain yönetimi (Bütün sorunları çözen satır)
    kotlin { 
        jvmToolchain(25) 
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
}
