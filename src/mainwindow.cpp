#include "mainwindow.h"
#include <QSettings>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QtMath>
#include <QStyle>
#include <QFontDatabase>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("ゴルトンボード シミュレータ");
    resize(960, 720);

    // アプリケーションフォントの設定（モダンなサンセリフ）
    QFont defaultFont("Segoe UI", 10);
    defaultFont.setStyleHint(QFont::SansSerif);
    setFont(defaultFont);

    setupUI();
    applyTheme();
    loadParams();

    // 初期状態の設定
    onRowCountChanged(m_rowSlider->value());
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // メインセントラルウィジェットと水平レイアウト
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // --- 左側: コントロールパネル ---
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(300);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);

    // シミュレーション制御グループ
    QGroupBox *ctrlGroup = new QGroupBox("シミュレーション制御", leftPanel);
    QVBoxLayout *ctrlLayout = new QVBoxLayout(ctrlGroup);
    ctrlLayout->setSpacing(12);

    m_playPauseBtn = new QPushButton("シミュレーション開始", ctrlGroup);
    m_playPauseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);

    m_resetBtn = new QPushButton("リセット", ctrlGroup);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    ctrlLayout->addWidget(m_playPauseBtn);
    ctrlLayout->addWidget(m_resetBtn);

    // パラメータ設定グループ
    QGroupBox *paramGroup = new QGroupBox("物理・環境パラメータ", leftPanel);
    QFormLayout *paramLayout = new QFormLayout(paramGroup);
    paramLayout->setVerticalSpacing(12);
    paramLayout->setHorizontalSpacing(10);

    // ピンの段数
    m_rowSlider = new QSlider(Qt::Horizontal, paramGroup);
    m_rowSlider->setRange(5, 18);
    m_rowSlider->setValue(10);
    m_rowValueLabel = new QLabel("10段", paramGroup);
    m_rowValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_rowValueLabel->setFixedWidth(40);
    connect(m_rowSlider, &QSlider::valueChanged, this, &MainWindow::onRowCountChanged);
    
    QHBoxLayout *rowSliderLayout = new QHBoxLayout();
    rowSliderLayout->addWidget(m_rowSlider);
    rowSliderLayout->addWidget(m_rowValueLabel);
    paramLayout->addRow("ピンの段数:", rowSliderLayout);

    // ボール生成速度
    m_dropRateSlider = new QSlider(Qt::Horizontal, paramGroup);
    m_dropRateSlider->setRange(1, 50);
    m_dropRateSlider->setValue(5);
    m_dropRateValueLabel = new QLabel("5個/秒", paramGroup);
    m_dropRateValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_dropRateValueLabel->setFixedWidth(50);
    connect(m_dropRateSlider, &QSlider::valueChanged, this, &MainWindow::onDropRateChanged);

    QHBoxLayout *dropRateSliderLayout = new QHBoxLayout();
    dropRateSliderLayout->addWidget(m_dropRateSlider);
    dropRateSliderLayout->addWidget(m_dropRateValueLabel);
    paramLayout->addRow("投入速度:", dropRateSliderLayout);

    // 重力
    m_gravitySlider = new QSlider(Qt::Horizontal, paramGroup);
    m_gravitySlider->setRange(100, 800);
    m_gravitySlider->setValue(350);
    m_gravityValueLabel = new QLabel("350 px/s²", paramGroup);
    m_gravityValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_gravityValueLabel->setFixedWidth(65);
    connect(m_gravitySlider, &QSlider::valueChanged, this, &MainWindow::onGravityChanged);

    QHBoxLayout *gravitySliderLayout = new QHBoxLayout();
    gravitySliderLayout->addWidget(m_gravitySlider);
    gravitySliderLayout->addWidget(m_gravityValueLabel);
    paramLayout->addRow("重力の強さ:", gravitySliderLayout);

    // 反発係数
    m_elasticitySlider = new QSlider(Qt::Horizontal, paramGroup);
    m_elasticitySlider->setRange(0, 85);
    m_elasticitySlider->setValue(50);
    m_elasticityValueLabel = new QLabel("0.50", paramGroup);
    m_elasticityValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_elasticityValueLabel->setFixedWidth(40);
    connect(m_elasticitySlider, &QSlider::valueChanged, this, &MainWindow::onElasticityChanged);

    QHBoxLayout *elasticitySliderLayout = new QHBoxLayout();
    elasticitySliderLayout->addWidget(m_elasticitySlider);
    elasticitySliderLayout->addWidget(m_elasticityValueLabel);
    paramLayout->addRow("ピンの弾性:", elasticitySliderLayout);

    // 右に進む確率 p
    m_biasSlider = new QSlider(Qt::Horizontal, paramGroup);
    m_biasSlider->setRange(1, 99);
    m_biasSlider->setValue(50);
    m_biasValueLabel = new QLabel("p = 0.50", paramGroup);
    m_biasValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_biasValueLabel->setFixedWidth(65);
    connect(m_biasSlider, &QSlider::valueChanged, this, &MainWindow::onBiasChanged);

    QHBoxLayout *biasSliderLayout = new QHBoxLayout();
    biasSliderLayout->addWidget(m_biasSlider);
    biasSliderLayout->addWidget(m_biasValueLabel);
    paramLayout->addRow("右確率 p:", biasSliderLayout);

    // ボールサイズ
    m_ballSizeSlider = new QSlider(Qt::Horizontal, paramGroup);
    m_ballSizeSlider->setRange(50, 200);
    m_ballSizeSlider->setValue(100);
    m_ballSizeValueLabel = new QLabel("1.00x", paramGroup);
    m_ballSizeValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ballSizeValueLabel->setFixedWidth(45);
    connect(m_ballSizeSlider, &QSlider::valueChanged, this, &MainWindow::onBallSizeChanged);

    QHBoxLayout *ballSizeSliderLayout = new QHBoxLayout();
    ballSizeSliderLayout->addWidget(m_ballSizeSlider);
    ballSizeSliderLayout->addWidget(m_ballSizeValueLabel);
    paramLayout->addRow("ボールサイズ:", ballSizeSliderLayout);

    // シミュレーション速度
    m_speedCombo = new QComboBox(paramGroup);
    m_speedCombo->addItems({"標準速度 (1x)", "高速 (2x)", "超高速 (5x)", "極限 (10x)"});
    m_speedCombo->setCurrentIndex(0);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSpeedChanged);
    paramLayout->addRow("シミュ速度:", m_speedCombo);

    // ピンの配置
    m_layoutCombo = new QComboBox(paramGroup);
    m_layoutCombo->addItems({"三角形 (Triangle)", "四角格子 (Square Grid)", "ひし形 (Diamond)"});
    m_layoutCombo->setCurrentIndex(0);
    connect(m_layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onPegLayoutChanged);
    paramLayout->addRow("ピン配置:", m_layoutCombo);

    m_paramResetBtn = new QPushButton("初期値に戻す", paramGroup);
    m_paramResetBtn->setCursor(Qt::PointingHandCursor);
    connect(m_paramResetBtn, &QPushButton::clicked, this, &MainWindow::onParamResetClicked);

    m_paramSaveBtn = new QPushButton("現在値を保存", paramGroup);
    m_paramSaveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_paramSaveBtn, &QPushButton::clicked, this, &MainWindow::onParamSaveClicked);

    QHBoxLayout *paramBtnLayout = new QHBoxLayout();
    paramBtnLayout->addWidget(m_paramResetBtn);
    paramBtnLayout->addWidget(m_paramSaveBtn);
    paramLayout->addRow(paramBtnLayout);

    // 統計データ数値グループ
    QGroupBox *statGroup = new QGroupBox("リアルタイム統計メトリクス", leftPanel);
    QGridLayout *statLayout = new QGridLayout(statGroup);
    statLayout->setSpacing(12);

    QLabel *lbl1 = new QLabel("落下ボール総数:", statGroup);
    m_totalBallsLabel = new QLabel("0 個", statGroup);
    m_totalBallsLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #00c8a0;");

    QLabel *lbl2 = new QLabel("実測平均値:", statGroup);
    m_meanLabel = new QLabel("-", statGroup);

    QLabel *lbl3 = new QLabel("実測標準偏差:", statGroup);
    m_stdDevLabel = new QLabel("-", statGroup);

    statLayout->addWidget(lbl1, 0, 0);
    statLayout->addWidget(m_totalBallsLabel, 0, 1);
    statLayout->addWidget(lbl2, 1, 0);
    statLayout->addWidget(m_meanLabel, 1, 1);
    statLayout->addWidget(lbl3, 2, 0);
    statLayout->addWidget(m_stdDevLabel, 2, 1);

    leftLayout->addWidget(ctrlGroup);
    leftLayout->addWidget(paramGroup);
    leftLayout->addWidget(statGroup);
    leftLayout->addStretch();

    // --- 中央/右側: シミュレーション キャンバス ---
    m_simWidget = new SimulationWidget(this);
    m_simWidget->setMinimumWidth(500);

    // シミュレーションからのシグナル受信設定
    connect(m_simWidget, &SimulationWidget::ballDropped, this, &MainWindow::updateBallCountLabel);
    connect(m_simWidget, &SimulationWidget::distributionChanged, this, &MainWindow::updateStatistics);

    // レイアウトの組み立て
    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(m_simWidget, 1); // SimulationWidget が右側全体で伸縮
}



void MainWindow::applyTheme()
{
    // アプリ全体のダークテーマスタイルシート
    QString qss = R"(
        QMainWindow {
            background-color: #121216;
        }
        QWidget {
            color: #e0e0e8;
        }
        QGroupBox {
            background-color: #1a1a24;
            border: 1px solid #2d2d3d;
            border-radius: 8px;
            margin-top: 15px;
            padding-top: 15px;
            font-weight: bold;
            font-size: 11px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 5px;
            color: #00c8a0;
        }
        QLabel {
            font-size: 10pt;
        }
        QPushButton {
            background-color: #252535;
            color: #ffffff;
            border: 1px solid #3d3d52;
            border-radius: 6px;
            padding: 8px 12px;
            font-size: 10pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #323247;
            border-color: #00c8a0;
            color: #00c8a0;
        }
        QPushButton:pressed {
            background-color: #1d1d28;
        }
        QSlider::groove:horizontal {
            border: 1px solid #2d2d3d;
            height: 6px;
            background: #252535;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #00c8a0;
            border: 1px solid #00a882;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #ff00c8;
            border-color: #cc00a0;
        }
        QComboBox {
            background-color: #252535;
            color: #ffffff;
            border: 1px solid #3d3d52;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 9.5pt;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 1px;
            border-left-color: #3d3d52;
            border-left-style: solid;
        }
    )";

    setStyleSheet(qss);

}



void MainWindow::onPlayPauseClicked()
{
    if (!m_isPlaying) {
        m_simWidget->start();
        m_isPlaying = true;
        m_playPauseBtn->setText("一時停止");
        m_playPauseBtn->setStyleSheet("QPushButton { background-color: #851c1c; border-color: #ff4a4a; } QPushButton:hover { background-color: #a32424; border-color: #ff6e6e; color: white; }");
    } else {
        m_simWidget->pause();
        m_isPlaying = false;
        m_playPauseBtn->setText("再開");
        m_playPauseBtn->setStyleSheet("");
    }
}

void MainWindow::onResetClicked()
{
    m_simWidget->reset();
    m_isPlaying = false;
    m_playPauseBtn->setText("シミュレーション開始");
    m_playPauseBtn->setStyleSheet("");
}

void MainWindow::onRowCountChanged(int value)
{
    m_rowValueLabel->setText(QString("%1段").arg(value));
    m_simWidget->setRowCount(value);
    if (m_isPlaying) {
        m_isPlaying = false;
        m_playPauseBtn->setText("シミュレーション開始");
        m_playPauseBtn->setStyleSheet("");
    }
    updateStatistics(m_simWidget->getSlotCounts());
}

void MainWindow::onDropRateChanged(int value)
{
    m_dropRateValueLabel->setText(QString("%1個/秒").arg(value));
    m_simWidget->setBallDropRate(value);
}

void MainWindow::onGravityChanged(int value)
{
    m_gravityValueLabel->setText(QString("%1 px/s²").arg(value));
    m_simWidget->setGravity(value);
}

void MainWindow::onElasticityChanged(int value)
{
    double e = value / 100.0;
    m_elasticityValueLabel->setText(QString("%1").arg(e, 0, 'f', 2));
    m_simWidget->setElasticity(e);
}

void MainWindow::saveParams()
{
    QSettings s("GaltonBoard", "Simulator");
    s.setValue("rowCount",   m_rowSlider->value());
    s.setValue("dropRate",   m_dropRateSlider->value());
    s.setValue("gravity",    m_gravitySlider->value());
    s.setValue("elasticity", m_elasticitySlider->value());
    s.setValue("bias",       m_biasSlider->value());
    s.setValue("ballSize",   m_ballSizeSlider->value());
    s.setValue("speed",      m_speedCombo->currentIndex());
    s.setValue("layout",     m_layoutCombo->currentIndex());
}

void MainWindow::loadParams()
{
    QSettings s("GaltonBoard", "Simulator");
    if (!s.contains("rowCount")) return;

    m_rowSlider->setValue(       s.value("rowCount",   10).toInt());
    m_dropRateSlider->setValue(  s.value("dropRate",    5).toInt());
    m_gravitySlider->setValue(   s.value("gravity",   350).toInt());
    m_elasticitySlider->setValue(s.value("elasticity", 50).toInt());
    m_biasSlider->setValue(      s.value("bias",       50).toInt());
    m_ballSizeSlider->setValue(  s.value("ballSize", 100).toInt());
    m_speedCombo->setCurrentIndex( s.value("speed",   0).toInt());
    m_layoutCombo->setCurrentIndex(s.value("layout",  0).toInt());
}

void MainWindow::onParamSaveClicked()
{
    saveParams();
    m_paramSaveBtn->setText("保存しました");
    QTimer::singleShot(1500, this, [this]{ m_paramSaveBtn->setText("現在値を保存"); });
}

void MainWindow::onParamResetClicked()
{
    m_rowSlider->setValue(10);
    m_dropRateSlider->setValue(5);
    m_gravitySlider->setValue(350);
    m_elasticitySlider->setValue(50);
    m_biasSlider->setValue(50);
    m_ballSizeSlider->setValue(100);
    m_speedCombo->setCurrentIndex(0);
    m_layoutCombo->setCurrentIndex(0);
}

void MainWindow::onBiasChanged(int value)
{
    double p = value / 100.0;
    m_biasValueLabel->setText(QString("p = %1").arg(p, 0, 'f', 2));
    m_simWidget->setBiasProbability(p);
}

void MainWindow::onBallSizeChanged(int value)
{
    double factor = value / 100.0;
    m_ballSizeValueLabel->setText(QString("%1x").arg(factor, 0, 'f', 2));
    m_simWidget->setBallSize(factor);
}

void MainWindow::onSpeedChanged(int index)
{
    int speed = 1;
    switch (index) {
        case 0: speed = 1; break;
        case 1: speed = 2; break;
        case 2: speed = 5; break;
        case 3: speed = 10; break;
    }
    m_simWidget->setSimulationSpeed(speed);
}

void MainWindow::onPegLayoutChanged(int index)
{
    m_simWidget->setPegLayout(static_cast<PegLayout>(index));
    if (m_isPlaying) {
        m_isPlaying = false;
        m_playPauseBtn->setText("シミュレーション開始");
        m_playPauseBtn->setStyleSheet("");
    }
}

void MainWindow::updateBallCountLabel(int count)
{
    m_totalBallsLabel->setText(QString("%1 個").arg(count));
}

void MainWindow::updateStatistics(const QVector<int> &slotCounts)
{
    int totalBalls = m_simWidget->getTotalBallsDropped();

    // 数値統計の更新
    if (totalBalls == 0) {
        m_meanLabel->setText("-");
        m_stdDevLabel->setText("-");
        return;
    }

    // 両端マージン分のオフセットを考慮してインデックスを調整
    int offset = m_simWidget->slotOffset();

    // 実測平均の計算
    double empiricalSum = 0.0;
    for (int i = 0; i < slotCounts.size(); ++i) {
        empiricalSum += (i - offset) * slotCounts[i];
    }
    double empiricalMean = empiricalSum / totalBalls;

    // 実測標準偏差の計算
    double empiricalVarianceSum = 0.0;
    for (int i = 0; i < slotCounts.size(); ++i) {
        empiricalVarianceSum += std::pow((i - offset) - empiricalMean, 2) * slotCounts[i];
    }
    double empiricalStdDev = std::sqrt(empiricalVarianceSum / totalBalls);

    // ラベルへのセット
    m_meanLabel->setText(QString("%1").arg(empiricalMean, 0, 'f', 2));
    m_stdDevLabel->setText(QString("%1").arg(empiricalStdDev, 0, 'f', 2));
}
