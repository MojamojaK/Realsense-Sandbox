# Realsense Sandbox

## セットアップ

以下の手順を行って、"%userprofile%/Documents/GitHub/>" にビルド環境をセットアップする。


### Git(2.22.0)

```
https://gitforwindows.org
```


### CMake(3.15.0)

```
https://github.com/Kitware/CMake/releases/download/v3.15.0/cmake-3.15.0-win64-x64.msi
```

ダウンロードして、インストールする。
インストーラの途中で "Add CMake to the system PATH for all users" を選択する。


### Visual Studio Community 2017

```
https://visualstudio.microsoft.com/ja/downloads
```

ダウンロードして、インストールする。
インストーラの途中でワークロードの "C++ によるデスクトップ開発" にチェックを入れる。


### Clone this repository

```
cd %userprofile%/Documents/GitHub
mkdir Realsense-Sandbox && cd Realsense-Sandbox
git clone --recursive git@github.com:MojamojaK/Realsense-Sandbox.git
cd Realsense-Sandbox
```


### OpenCV + OpenCV Contrib (Aruco)

コマンドプロンプトで実行する。

```
cd third-party/opencv
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="%userprofile%/Binaries/OpenCV" -DOPENCV_EXTRA_MODULES_PATH="%cd%/../../opencv_contrib/modules" -DBUILD_EXAMPLES=OFF
cmake --build . --config release --target install -j 6
cd ../../..
```


### librealsense2
コマンドプロンプトで実行する。

```
cd third-party/librealsense
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="%userprofile%/Binaries/librealsense2" -DFORCE_RSUSB_BACKEND=false -DBUILD_EXAMPLES=false -DBUILD_WITH_OPENMP=false
cmake --build . --config release --target install -j 6
cd ../../..
```

### 環境変数の設定

システム環境変数のPathに以下を追加する。

```
%userprofile%\Binaries\OpenCV\x64\vc16\bin
%userprofile%\Binaries\librealsense2\bin
```


## プロジェクトのビルド

```
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config release
cd ..
```

ビルドタイプはReleaseにすること。
