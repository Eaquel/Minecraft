plugins {
    id("com.android.application")
    // org.jetbrains.kotlin.android eklentisi AGP 9.0 ile tamamen kaldırıldığı için buradan silindi.
}

android {
    namespace  = "com.omni.craft"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.omni.craft"
        minSdk        = 28
        targetSdk     = 36
        versionCode   = 1
        versionName   = "1.0.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
            version = "29.0.14206865"
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf(
                    "-std=c++26",
                    "-O3",
                    "-ffast-math",
                    "-funroll-loops",
                    "-fomit-frame-pointer",
                    "-fvisibility=hidden",
                    "-DANDROID_STL=c++_shared"
                )
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_ARM_NEON=ON",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DCMAKE_CXX_STANDARD=26",
                    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
                )
            }
        }
    }

    externalNativeBuild {
        cmake {
            path    = file("Source/Main/Native/CMakeLists.txt")
            version = "4.3.2"
        }
    }

    ndkVersion = "29.0.14206865"

    buildTypes {
        release {
            isMinifyEnabled   = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            ndk {
                debugSymbolLevel = "FULL"
            }
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                    cppFlags  += listOf("-O3", "-DNDEBUG")
                }
            }
        }
        debug {
            isMinifyEnabled = false
            isDebuggable    = true
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Debug"
                    cppFlags  += listOf("-O0", "-g", "-DDEBUG")
                }
            }
        }
    }

    sourceSets["main"].apply {
        manifest.srcFile("Source/Main/AndroidManifests.xml")
        kotlin.srcDirs("Source/Main/Kotlin")
        res.srcDirs("Source/Main/Res")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_25
        targetCompatibility = JavaVersion.VERSION_25
    }

    kotlinOptions {
        jvmTarget = "25"
        freeCompilerArgs += listOf(
            "-Xopt-in=kotlin.RequiresOptIn",
            "-Xjvm-default=all",
            "-opt-in=kotlin.ExperimentalStdlibApi"
        )
    }

    packaging {
        resources {
            excludes += setOf(
                "/META-INF/{AL2.0,LGPL2.1}",
                "/META-INF/DEPENDENCIES",
                "/*.txt"
            )
        }
        jniLibs {
            useLegacyPackaging = false
        }
    }

    splits {
        abi {
            isEnable       = true
            reset()
            include("arm64-v8a", "armeabi-v7a", "x86_64")
            isUniversalApk = true
        }
    }

    lint {
        abortOnError       = false
        checkReleaseBuilds = true
    }

    buildFeatures {
        buildConfig = true
    }
}

dependencies {
    implementation("org.jetbrains.kotlin:kotlin-stdlib:2.3.21")
    implementation("androidx.core:core-ktx:1.16.0")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.10.2")
}
