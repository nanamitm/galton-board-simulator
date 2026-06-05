#include "simulationwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QtMath>
#include <QDebug>

SimulationWidget::SimulationWidget(QWidget *parent)
    : QWidget(parent)
{
    // 背景の更新頻度を設定 (タイマーで定期更新)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SimulationWidget::updateSimulation);
    m_timer->setInterval(16); // ~60 FPS

    initializeBoard();
}

SimulationWidget::~SimulationWidget()
{
}

void SimulationWidget::start()
{
    m_isRunning = true;
    m_timer->start();
}

void SimulationWidget::pause()
{
    m_isRunning = false;
    m_timer->stop();
}

void SimulationWidget::reset()
{
    m_isRunning = false;
    m_timer->stop();
    m_balls.clear();
    m_totalBallsDropped = 0;
    m_spawnAccumulatorMs = 0;
    
    // スロットのカウントクリア
    std::fill(m_slotCounts.begin(), m_slotCounts.end(), 0);

    // ピンの発光をリセット
    for (auto &peg : m_pegs) {
        peg.glowIntensity = 0.0;
    }

    emit ballDropped(0);
    emit distributionChanged(m_slotCounts);
    update();
}

void SimulationWidget::step()
{
    // ボール生成（updateSimulation と同じ蓄積ロジック）
    m_spawnAccumulatorMs += 16;
    int spawnIntervalMs = 1000 / m_ballDropRate;
    if (m_spawnAccumulatorMs >= spawnIntervalMs) {
        spawnBall();
        m_spawnAccumulatorMs = 0;
    }

    // 1フレーム分物理演算を進める
    updatePhysics(0.016);
    checkCollisions(0.016);

    for (auto &peg : m_pegs) {
        peg.glowIntensity = std::max(0.0, peg.glowIntensity - 0.05);
    }

    update();
}

void SimulationWidget::setRowCount(int count)
{
    if (m_rowCount != count) {
        m_rowCount = count;
        reset();
        initializeBoard();
    }
}

void SimulationWidget::setBallDropRate(int countPerSec)
{
    m_ballDropRate = countPerSec;
}

void SimulationWidget::setGravity(double g)
{
    m_gravity = g;
}

void SimulationWidget::setElasticity(double e)
{
    m_elasticity = qBound(0.0, e, 1.0);
}

void SimulationWidget::setBallSize(double factor)
{
    m_ballSizeFactor = qBound(0.5, factor, 2.0);
    m_ballSize = qMax(3.0, m_pegSpacing * 0.18 * m_ballSizeFactor);
}

void SimulationWidget::setSimulationSpeed(int speed)
{
    m_simulationSpeed = qBound(1, speed, 10);
}

void SimulationWidget::setBiasProbability(double p)
{
    m_biasProbability = qBound(0.01, p, 0.99);
    update();
}

void SimulationWidget::setPegLayout(PegLayout layout)
{
    if (m_pegLayout != layout) {
        m_pegLayout = layout;
        reset();
        initializeBoard();
    }
}

void SimulationWidget::initializeBoard()
{
    m_pegs.clear();
    m_slotCounts.resize(m_rowCount + 1 + 2 * kSlotMargin);
    std::fill(m_slotCounts.begin(), m_slotCounts.end(), 0);

    // 画面サイズに応じてピンの間隔を自動計算
    double gridHeightAvailable = height() - m_topMargin - m_bottomMargin;
    m_pegSpacing = gridHeightAvailable / (m_rowCount + 5.0) / 0.866;
    
    // 間隔の最大値を 23.0 まで小さく抑え、非常に密なピン配置を維持します
    m_pegSpacing = qBound(12.0, m_pegSpacing, 23.0);

    // ボールとピンのサイズを間隔に比例して自動スケーリング (ユーザー倍率を反映)
    m_ballSize = qMax(3.0, m_pegSpacing * 0.18 * m_ballSizeFactor);
    double pegRadius = qMax(1.2, m_pegSpacing * 0.08);

    double centerX = width() / 2.0;
    double H = m_pegSpacing * 0.866; // 高さ

    if (m_pegLayout == Triangle) {
        // 台形配置：各行の三角形ピンの両端に kSlotMargin 個のバッファピンを追加。
        // 物理的には台形だが中央の三角形部分のみ通常表示し、端は半透明。
        for (int r = 0; r < m_rowCount; ++r) {
            int centralPins = r + 1;
            double centralStartX = centerX - (r * m_pegSpacing) / 2.0;
            double y = m_topMargin + r * H;

            // 左バッファピン
            for (int m = kSlotMargin; m >= 1; --m) {
                Peg peg;
                peg.pos = QPointF(centralStartX - m * m_pegSpacing, y);
                peg.radius = pegRadius;
                peg.glowIntensity = 0.0;
                peg.isMargin = true;
                m_pegs.push_back(peg);
            }

            // 中央（三角形）ピン
            for (int c = 0; c < centralPins; ++c) {
                Peg peg;
                peg.pos = QPointF(centralStartX + c * m_pegSpacing, y);
                peg.radius = pegRadius;
                peg.glowIntensity = 0.0;
                peg.isMargin = false;
                m_pegs.push_back(peg);
            }

            // 右バッファピン
            for (int m = 1; m <= kSlotMargin; ++m) {
                Peg peg;
                peg.pos = QPointF(centralStartX + (centralPins - 1 + m) * m_pegSpacing, y);
                peg.radius = pegRadius;
                peg.glowIntensity = 0.0;
                peg.isMargin = true;
                m_pegs.push_back(peg);
            }
        }
    }
    else if (m_pegLayout == SquareGrid) {
        // 四角格子配置 (RowCount x RowCount)
        double startX = centerX - ((m_rowCount - 1) * m_pegSpacing) / 2.0;
        for (int r = 0; r < m_rowCount; ++r) {
            double y = m_topMargin + r * H;
            for (int c = 0; c < m_rowCount; ++c) {
                Peg peg;
                peg.pos = QPointF(startX + c * m_pegSpacing, y);
                peg.radius = pegRadius;
                peg.glowIntensity = 0.0;
                m_pegs.push_back(peg);
            }
        }
    }
    else if (m_pegLayout == Diamond) {
        // ひし形（ダイヤモンド）配置
        for (int r = 0; r < m_rowCount; ++r) {
            // 行 r におけるピンの個数 K
            int pegCount = (r < m_rowCount / 2) ? (r + 1) : (m_rowCount - r);
            double startX = centerX - ((pegCount - 1) * m_pegSpacing) / 2.0;
            double y = m_topMargin + r * H;

            for (int c = 0; c < pegCount; ++c) {
                Peg peg;
                peg.pos = QPointF(startX + c * m_pegSpacing, y);
                peg.radius = pegRadius;
                peg.glowIntensity = 0.0;
                m_pegs.push_back(peg);
            }
        }
    }

    // スロットの基準座標
    m_slotTopY = m_topMargin + m_rowCount * H + 10.0;
    m_gridLeft = width() / 2.0 - (m_rowCount * m_pegSpacing) / 2.0 - kSlotMargin * m_pegSpacing;
}

void SimulationWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 画面リサイズ時にボードを再構成 (ボールは維持したいため、初期化処理を部分的に実行)
    int prevDropped = m_totalBallsDropped;
    QVector<int> prevCounts = m_slotCounts;

    initializeBoard();

    // カウントは復元
    m_totalBallsDropped = prevDropped;
    m_slotCounts = prevCounts;
}

void SimulationWidget::spawnBall()
{
    if (m_pegs.isEmpty() || m_balls.size() >= kMaxBalls) return;

    Ball ball;

    // 最も高い位置（最小のy座標）にあるピン群を特定する
    double minY = m_pegs[0].pos.y();
    for (const auto &peg : m_pegs) {
        if (peg.pos.y() < minY) {
            minY = peg.pos.y();
        }
    }

    // 画面中心（centerX）に最も近いピンを最上部から選ぶ
    double centerX = width() / 2.0;
    QPointF spawnPeg = m_pegs[0].pos;
    double minDistanceX = std::abs(spawnPeg.x() - centerX);

    for (const auto &peg : m_pegs) {
        if (std::abs(peg.pos.y() - minY) < 1.0) { // 最上部のピンのみ
            double dist = std::abs(peg.pos.x() - centerX);
            if (dist < minDistanceX) {
                minDistanceX = dist;
                spawnPeg = peg.pos;
            }
        }
    }

    double noise = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.8; // ブレを小さくして直撃させる
    double H = m_pegSpacing * 0.866;
    
    ball.pos = QPointF(spawnPeg.x() + noise, spawnPeg.y() - H * 0.6);
    ball.vel = QPointF(0.0, 10.0); // 下向き初期速度
    ball.radius = m_ballSize;

    // ネオンカラーの美しいランダムカラー
    int hue = QRandomGenerator::global()->bounded(180, 320);
    ball.color = QColor::fromHsl(hue, 180, 130);

    m_balls.push_back(ball);
}

void SimulationWidget::updateSimulation()
{
    if (!m_isRunning) return;

    double dt = 0.016; // 60 FPS 基準

    // ボール生成は実時間ベースで1フレームに1回（シミュ速度に乗算しない）
    m_spawnAccumulatorMs += 16;
    int spawnIntervalMs = 1000 / m_ballDropRate;
    if (m_spawnAccumulatorMs >= spawnIntervalMs) {
        spawnBall();
        m_spawnAccumulatorMs = 0;
    }

    // 物理演算のみシミュ速度倍率分繰り返す
    for (int step = 0; step < m_simulationSpeed; ++step) {
        updatePhysics(dt);
        checkCollisions(dt);
    }

    // ピンの光減衰処理
    for (auto &peg : m_pegs) {
        peg.glowIntensity = std::max(0.0, peg.glowIntensity - 0.05);
    }

    update();
}

void SimulationWidget::updatePhysics(double dt)
{
    for (auto &ball : m_balls) {
        // 重力の適用
        ball.vel.ry() += m_gravity * dt;

        // 微小な空気抵抗
        ball.vel *= (1.0 - 0.08 * dt);

        // 位置の更新
        ball.pos += ball.vel * dt;
    }
}

void SimulationWidget::checkCollisions(double dt)
{
    for (auto it = m_balls.begin(); it != m_balls.end(); ) {
        Ball &ball = *it;
        bool ballRemoved = false;

        // 1. ピンとの衝突判定
        for (auto &peg : m_pegs) {
            double dx = ball.pos.x() - peg.pos.x();
            double dy = ball.pos.y() - peg.pos.y();
            double dist = std::sqrt(dx*dx + dy*dy);
            double minDist = ball.radius + peg.radius;

            if (dist < minDist && dist > 1e-9) {
                // 衝突ベクトル
                double nx = dx / dist;
                double ny = dy / dist;

                // めり込み防止（位置の押し出し）
                ball.pos.setX(peg.pos.x() + nx * minDist);
                ball.pos.setY(peg.pos.y() + ny * minDist);

                // 法線方向の相対速度
                double vn = ball.vel.x() * nx + ball.vel.y() * ny;

                if (vn < 0) {
                    // y方向は物理反射（重力との整合性を保つ）
                    ball.vel.rx() -= (1.0 + m_elasticity) * vn * nx;
                    ball.vel.ry() -= (1.0 + m_elasticity) * vn * ny;

                    // x方向: p確率で右、(1-p)確率で左に進む
                    bool goRight = (QRandomGenerator::global()->generateDouble() < m_biasProbability);
                    ball.vel.setX(goRight ? m_pegSpacing * 2.5 : -m_pegSpacing * 2.5);

                    peg.glowIntensity = 1.0;
                }
            }
        }

        // 2. 左右の壁との衝突 (スロットの両端に壁を合わせることで、玉がヒストグラムの外に飛び出すのを防ぎます)
        double leftWall = m_gridLeft;
        double rightWall = m_gridLeft + (m_rowCount + 1 + 2 * kSlotMargin) * m_pegSpacing;
        if (ball.pos.x() < leftWall + ball.radius) {
            ball.pos.setX(leftWall + ball.radius);
            ball.vel.setX(-ball.vel.x() * m_elasticity);
        } else if (ball.pos.x() > rightWall - ball.radius) {
            ball.pos.setX(rightWall - ball.radius);
            ball.vel.setX(-ball.vel.x() * m_elasticity);
        }

        // 3. スロット底面への進入判定
        if (ball.pos.y() >= m_slotTopY) {
            // スロットのインデックス計算
            int binIdx = qFloor((ball.pos.x() - m_gridLeft) / m_pegSpacing);
            binIdx = qBound(0, binIdx, m_rowCount + 2 * kSlotMargin);

            // カウントをインクリメント
            m_slotCounts[binIdx]++;
            m_totalBallsDropped++;

            // シグナル通知
            emit ballDropped(m_totalBallsDropped);
            emit distributionChanged(m_slotCounts);

            // リストからボールを削除
            it = m_balls.erase(it);
            ballRemoved = true;
        }

        if (!ballRemoved) {
            ++it;
        }
    }
}

void SimulationWidget::drawGlowEffect(QPainter &painter, const QPointF &center, double radius, const QColor &color, double intensity)
{
    // ピンが衝突時にふわっと光るエフェクト
    QRadialGradient gradient(center, radius * 3.5);
    QColor glowColor = color;
    glowColor.setAlpha(static_cast<int>(80 * intensity));
    gradient.setColorAt(0.0, glowColor);
    gradient.setColorAt(1.0, Qt::transparent);

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius * 3.5, radius * 3.5);
}

void SimulationWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. 背景のグラデーション描画 (深みのあるダークネイビー)
    QLinearGradient bgGradient(0, 0, 0, height());
    bgGradient.setColorAt(0.0, QColor(22, 22, 29));
    bgGradient.setColorAt(1.0, QColor(10, 10, 14));
    painter.fillRect(rect(), bgGradient);

    double bottomY = height() - m_bottomMargin;

    // 2. スロットの仕切り（壁）の描画
    int totalSlots = m_rowCount + 1 + 2 * kSlotMargin;
    painter.setPen(QPen(QColor(255, 255, 255, 25), 1.5, Qt::SolidLine));
    for (int i = 0; i <= totalSlots; ++i) {
        double x = m_gridLeft + i * m_pegSpacing;
        painter.drawLine(QPointF(x, m_slotTopY), QPointF(x, bottomY));
    }
    // 底面ライン
    painter.drawLine(QPointF(m_gridLeft, bottomY), QPointF(m_gridLeft + totalSlots * m_pegSpacing, bottomY));

    // 3. ピンの描画
    for (const auto &peg : m_pegs) {
        int alpha = peg.isMargin ? 70 : 255; // バッファピンは半透明

        if (peg.glowIntensity > 0.0 && !peg.isMargin) {
            drawGlowEffect(painter, peg.pos, peg.radius, QColor(0, 200, 160), peg.glowIntensity);
        }

        QRadialGradient pegGrad(peg.pos, peg.radius);
        if (peg.glowIntensity > 0.0 && !peg.isMargin) {
            pegGrad.setColorAt(0.0, QColor(220, 220, 220, alpha));
            pegGrad.setColorAt(1.0, QColor(0, 200, 160, alpha));
        } else {
            pegGrad.setColorAt(0.0, QColor(180, 180, 200, alpha));
            pegGrad.setColorAt(1.0, QColor(70, 70, 80, alpha));
        }
        painter.setBrush(pegGrad);
        painter.setPen(QPen(QColor(30, 30, 40, alpha), 0.5));
        painter.drawEllipse(peg.pos, peg.radius, peg.radius);
    }

    // 4. スロットの下部ヒストグラム描画
    int maxCount = 0;
    for (int count : m_slotCounts) {
        if (count > maxCount) maxCount = count;
    }

    double slotHeight = bottomY - m_slotTopY;
    for (int i = 0; i < totalSlots; ++i) {
        double x_left = m_gridLeft + i * m_pegSpacing;
        int count = m_slotCounts[i];
        
        if (count > 0 && maxCount > 0) {
            // 最大値に基づいて高さを決定 (最大でスロットの高さの80%まで)
            double ratio = static_cast<double>(count) / maxCount;
            double barHeight = ratio * (slotHeight * 0.80);

            // 美しく光るグラデーション（ネオンシアン）
            QLinearGradient barGrad(x_left + m_pegSpacing / 2.0, bottomY, 
                                     x_left + m_pegSpacing / 2.0, bottomY - barHeight);
            barGrad.setColorAt(0.0, QColor(0, 190, 150, 20));
            barGrad.setColorAt(1.0, QColor(0, 190, 150, 130));

            painter.setBrush(barGrad);
            painter.setPen(QPen(QColor(0, 190, 150), 1.0));
            
            // 棒グラフを描画（少しスリムにして隙間を作る）
            double padding = 2.0;
            QRectF barRect(x_left + padding, bottomY - barHeight, m_pegSpacing - padding * 2.0, barHeight);
            painter.drawRoundedRect(barRect, 3.0, 3.0);

            // 個数のテキスト描画
            painter.setPen(QColor(150, 150, 165));
            QFont font = painter.font();
            font.setPixelSize(qMax(9, static_cast<int>(m_pegSpacing * 0.3)));
            painter.setFont(font);
            painter.drawText(QRectF(x_left, bottomY - barHeight - 16, m_pegSpacing, 14), 
                             Qt::AlignCenter, QString::number(count));
        }
    }

    // 5. 理論分布曲線 B(n, p) のオーバーレイ描画
    {
        int n = m_rowCount;
        double p = m_biasProbability;

        // 動的計画法で P(X=k) ~ Binomial(n, p) を計算
        QVector<double> prob(n + 1, 0.0);
        prob[0] = 1.0;
        for (int i = 0; i < n; ++i) {
            for (int k = i + 1; k >= 1; --k)
                prob[k] = prob[k] * (1.0 - p) + prob[k - 1] * p;
            prob[0] *= (1.0 - p);
        }

        double maxProb = 0.0;
        for (double v : prob) if (v > maxProb) maxProb = v;

        double slotH = bottomY - m_slotTopY;
        double scale = (slotH * 0.80) / maxProb;

        // 各点の座標を事前計算
        QVector<QPointF> pts(n + 1);
        for (int k = 0; k <= n; ++k) {
            double xc = m_gridLeft + (kSlotMargin + k + 0.5) * m_pegSpacing;
            pts[k] = QPointF(xc, bottomY - prob[k] * scale);
        }

        // Catmull-Rom → Cubic Bezier 変換で滑らかな曲線を構築
        QPainterPath curvePath;
        curvePath.moveTo(pts[0]);
        for (int k = 0; k < n; ++k) {
            QPointF p0 = (k > 0)      ? pts[k - 1] : pts[0];
            QPointF p1 = pts[k];
            QPointF p2 = pts[k + 1];
            QPointF p3 = (k + 2 <= n) ? pts[k + 2] : pts[n];
            QPointF cp1 = p1 + (p2 - p0) / 6.0;
            QPointF cp2 = p2 - (p3 - p1) / 6.0;
            curvePath.cubicTo(cp1, cp2, p2);
        }

        // 塗りつぶし領域（曲線パスを底辺で閉じる）
        QPainterPath fillPath = curvePath;
        fillPath.lineTo(pts[n].x(), bottomY);
        fillPath.lineTo(pts[0].x(), bottomY);
        fillPath.closeSubpath();

        painter.setBrush(QColor(200, 170, 0, 18));
        painter.setPen(Qt::NoPen);
        painter.drawPath(fillPath);

        // 曲線本体
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(200, 170, 0, 180), 1.5));
        painter.drawPath(curvePath);

        // 各理論値に点
        painter.setBrush(QColor(200, 170, 0, 200));
        painter.setPen(Qt::NoPen);
        for (const QPointF &pt : pts)
            painter.drawEllipse(pt, 2.5, 2.5);
    }

    // 6. 落下中のボールの描画
    for (const auto &ball : m_balls) {
        // ボール本体の描画（ハイライトが入った立体的なグラデーション）
        QRadialGradient ballGrad(ball.pos - QPointF(ball.radius * 0.3, ball.radius * 0.3), ball.radius);
        ballGrad.setColorAt(0.0, QColor(255, 255, 255));
        ballGrad.setColorAt(0.2, ball.color);
        ballGrad.setColorAt(1.0, ball.color.darker(200));

        painter.setBrush(ballGrad);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(ball.pos, ball.radius, ball.radius);
    }
}
