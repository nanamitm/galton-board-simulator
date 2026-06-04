cmake_minimum_required(VERSION 3.16)
project(GaltonBoardSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtのMOC、RCC、UICを自動的に処理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 必要なQtコンポーネントを検索
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ソースファイルの定義
set(SOURCES
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/simulationwidget.h
    src/simulationwidget.cpp
)

# 実行ファイルの追加
add_executable(GaltonBoardSimulator ${SOURCES})

# src/ をインクルードパスに追加（相対インクルードを解決するため）
target_include_directories(GaltonBoardSimulator PRIVATE src)

# 依存ライブラリのリンク
target_link_libraries(GaltonBoardSimulator PRIVATE
    Qt6::Widgets
)

# Windows用の追加設定（コンソールを非表示にするため、リリースビルド等で有効）
if(WIN32)
    set_target_properties(GaltonBoardSimulator PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()
