#include <QApplication>
#include "mainwindow.h"

#ifdef Q_OS_WASM
#include <QFontDatabase>
#include <QFont>
#include <QScreen>

// Qt for WebAssembly は DejaVu しか同梱しておらず CJK が豆腐になるため、
// UI で使う文字だけに絞った Noto Sans JP を読み込む
// （resources/fonts/、tools/subset_font.py で生成）。
static void loadJapaneseFont(QApplication& app)
{
    const int id = QFontDatabase::addApplicationFont(
        ":/fonts/resources/fonts/NotoSansJP-subset.ttf");
    if (id < 0) {
        qWarning("failed to load the bundled Japanese font");
        return;
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) return;

    QFont font = app.font();
    font.setFamily(families.first());
    app.setFont(font);
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
#ifdef Q_OS_WASM
    loadJapaneseFont(a);
#endif

    MainWindow w;
#ifdef Q_OS_WASM
    // ブラウザの表示領域いっぱいに広げ、リサイズにも追従する
    // （Qt for WebAssembly はウィンドウを自動では追従させない）
    if (QScreen* screen = a.primaryScreen()) {
        w.setGeometry(screen->geometry());
        QObject::connect(screen, &QScreen::geometryChanged,
                         &w, [&w](const QRect& g) { w.setGeometry(g); });
    }
#endif
    w.show();
    
    return a.exec();
}
