#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include "simulationwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // UIシグナル受信用スロット
    void onPlayPauseClicked();
    void onResetClicked();
    void onParamResetClicked();
    void onParamSaveClicked();
    void onRowCountChanged(int value);
    void onDropRateChanged(int value);
    void onGravityChanged(int value);
    void onElasticityChanged(int value);
    void onBiasChanged(int value);
    void onSpeedChanged(int index);
    void onPegLayoutChanged(int index);

    // シミュレーション情報受信用スロット
    void updateStatistics(const QVector<int> &slotCounts);
    void updateBallCountLabel(int count);

private:
    void setupUI();
    void applyTheme();
    void saveParams();
    void loadParams();

    // ウィジェット構造
    SimulationWidget *m_simWidget;

    // コントロールUI要素
    QPushButton *m_playPauseBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_paramResetBtn;
    QPushButton *m_paramSaveBtn;
    
    QSlider *m_rowSlider;
    QLabel *m_rowValueLabel;

    QSlider *m_dropRateSlider;
    QLabel *m_dropRateValueLabel;

    QSlider *m_gravitySlider;
    QLabel *m_gravityValueLabel;

    QSlider *m_elasticitySlider;
    QLabel *m_elasticityValueLabel;

    QSlider *m_biasSlider;
    QLabel *m_biasValueLabel;

    QComboBox *m_speedCombo;
    QComboBox *m_layoutCombo;

    // 統計UI要素
    QLabel *m_totalBallsLabel;
    QLabel *m_meanLabel;
    QLabel *m_stdDevLabel;

    bool m_isPlaying = false;


};

#endif // MAINWINDOW_H
