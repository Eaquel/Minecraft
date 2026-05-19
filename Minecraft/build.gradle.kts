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
            manifest.srcFile("Source/Main/AndroidManifests.xml")
            // AGP 9.2+ uyumlu modern tekil tanımlama
            java.srcDir("Source/Main/Kotlin")
            res.srcDir("Source/Main/Res")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("Source/Main/Native/CMakeLists.txt")
            version = "4.3.2"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    tasks.withType<org.jetbrains.kotlin.gradle.tasks.KotlinCompile>().configureEach {
        compilerOptions {
            jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_25)
            freeCompilerArgs.add("-Xjdk-release=25")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
}
