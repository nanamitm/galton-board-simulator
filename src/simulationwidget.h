#ifndef SIMULATIONWIDGET_H
#define SIMULATIONWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QColor>
// シミュレーション内のボール構造体
struct Ball {
    QPointF pos;
    QPointF vel;
    double radius;
    QColor color;
};

enum PegLayout {
    Triangle = 0,
    SquareGrid = 1,
    Diamond = 2
};

// ピン（釘）構造体
struct Peg {
    QPointF pos;
    double radius;
    double glowIntensity = 0.0; // 衝突時の光る強さ (0.0 〜 1.0)
};

class SimulationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SimulationWidget(QWidget *parent = nullptr);
    ~SimulationWidget();

    // シミュレーション制御
    void start();
    void pause();
    void reset();
    void step(); // 1ステップのみ進める

    // パラメータセッター・ゲッター
    void setRowCount(int count);
    int rowCount() const { return m_rowCount; }

    void setBallDropRate(int countPerSec); // 1秒あたりのボール数
    int ballDropRate() const { return m_ballDropRate; }

    void setGravity(double g);
    double gravity() const { return m_gravity; }

    void setElasticity(double e);
    double elasticity() const { return m_elasticity; }

    void setBallSize(double size);
    double ballSize() const { return m_ballSize; }

    void setSimulationSpeed(int speed); // 1x, 2x, 5x, 10x
    int simulationSpeed() const { return m_simulationSpeed; }

    void setPegLayout(PegLayout layout);
    PegLayout pegLayout() const { return m_pegLayout; }

    void setBiasProbability(double p);
    double biasProbability() const { return m_biasProbability; }





    // 統計データ取得
    QVector<int> getSlotCounts() const { return m_slotCounts; }
    int getTotalBallsDropped() const { return m_totalBallsDropped; }
    int slotOffset() const { return kSlotMargin; }

signals:
    void ballDropped(int totalDropped);
    void distributionChanged(const QVector<int> &slotCounts);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateSimulation();

private:
    void initializeBoard();
    void spawnBall();
    void updatePhysics(double dt);
    void checkCollisions(double dt);
    void drawGlowEffect(QPainter &painter, const QPointF &center, double radius, const QColor &color, double intensity);

    // シミュレーションループ制御
    QTimer *m_timer;
    bool m_isRunning = false;
    int m_simulationSpeed = 1; // 1x, 2x, 5x...

    // 物理パラメータ
    int m_rowCount = 10;
    int m_ballDropRate = 5; // balls/sec
    double m_gravity = 350.0; // pixels/sec^2
    double m_elasticity = 0.5; // 反発係数
    double m_ballSize = 6.0;   // ボール半径
    double m_biasProbability = 0.5; // 右に進む確率

    // 構成要素
    QVector<Peg> m_pegs;
    QVector<Ball> m_balls;
    QVector<int> m_slotCounts; // 各スロットのボール積算数
    PegLayout m_pegLayout = Triangle;

    static constexpr int kMaxBalls = 500;
    static constexpr int kSlotMargin = 2; // 両端に追加するスロット数

    // 定規・グリッド寸法（リサイズ時に再計算）
    double m_pegSpacing = 40.0;
    double m_topMargin = 50.0;
    double m_bottomMargin = 60.0;
    double m_sideMargin = 40.0;
    double m_slotTopY = 0.0;
    double m_gridLeft = 0.0;

    // 統計変数
    int m_totalBallsDropped = 0;
    int m_spawnAccumulatorMs = 0; // ボール生成タイマー蓄積用
};

#endif // SIMULATIONWIDGET_H
